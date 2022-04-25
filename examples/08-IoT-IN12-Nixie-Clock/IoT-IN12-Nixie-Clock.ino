// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// IoT IN12 Nixie CLock
// This example demonstrates how to use Arduino IoT Cloud Dashboard
// and optional Amazon Alexa Assistant to control IoT IN12 Nixie Clock
// Control options: 
// - ON/OFF Nixie Clock, 
// - Backlight - ON/OFF, Color, Brightness.
// 
// Serial monitor is required to debug Arduino IoT Cloud connection
//
// Hardware:
// WiFi signal
// IN12 Nixie Clock - https://nixietester.com/project/iot-nixie-clock-shield-for-arduino
// Arduino Nano IoT 33 - https://store.arduino.cc/arduino-nano-33-iot
// Nixie Power Supply Module
// Nixie clock require 12V, 1A power supply

#include "arduino_secrets.h"
#include "thingProperties.h"

#include <Adafruit_NeoPixel.h>
// https://github.com/adafruit/Adafruit_NeoPixel
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

#include <RTCZero.h>
RTCZero main_rtc;             // RTC inside Nano 33 IoT

#include <RTClib.h>           // https://github.com/adafruit/RTClib
RTC_DS3231 ds3231_rtc;        // RTC DS3231 library declaration

uint32_t epochTime = 0;
uint8_t timeHour = 0;
uint8_t timeMinute = 0;
uint8_t timeSecond = 0;
                           

// Choose Time Format *******************************************************
#define hourFormat        24     // 12 Hour Clock or 24 Hour Clock
// **************************************************************************

// https://en.wikipedia.org/wiki/List_of_time_zones_by_country
// Choose your Time Zone ****************************************************
#define timeZone          2
// ************************************************************************** 

// Choose your hour to synchronize the Time via WiFi ************************
// The RTC DS3231 always works in 24 hour mode so if you want to set 3:00AM 
// use "3" if you want to set 14:00 or 2:00PM use "14" etc. 
#define timeToSynchronizeTime     3     // 3:00AM              
// **************************************************************************

// How many times to try to synchronize the Time
uint8_t numberOfTries = 0; 
uint8_t maxTries = 30;

// Millis time start
uint32_t millis_start = 0;

uint16_t current_hue_Value = 0;
uint8_t current_sat_Value = 0;
uint8_t current_bri_Value = 0;

boolean status_nixie_clock = false;
boolean status_backlight = false;
boolean timeToSynchronizeTimeFlag = 0;

// Bit states for Nixie Power Supply module control
#define ON          1          
#define OFF         0 

byte EN   = 32;       // Enable bit number for NPS Nixie Power Supply module 

// NeoPixels LEDs pin
#define LED_PIN       A0

// Number of NeoPixels LEDs
#define LED_COUNT     6
 
// Declare our NeoPixel led object:
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define dataPin     9                     // HCT595 shift register serial data pin             
#define clockPin    8                     // Clock pin
#define latchPin    7                     // Latch pin

// Bit array for 6 nixie tubes, dot1, dot2, EN
boolean nixieDisplayArray[64];

byte digit1, digit2, digit3, digit4, digit5, digit6;

// Cathodes assignment to the position in the 64 bit array
// Each cathode of nixie tubes is connected to the corresponding output of the corresponding shift register
// Bit numbers
byte nixie1[]={
//   0   1   2   3   4   5   6   7   8   9  
     5,  4,  3,  2,  1,  0, 15, 14,  7,  6  };
byte nixie2[]={
//   0   1   2   3   4   5   6   7   8   9
    22, 21, 20, 13, 12, 11, 10,  9,  8, 23  };
byte nixie3[]={
//   0   1   2   3   4   5   6   7   8   9  
    27, 25, 19, 18, 17, 16, 31, 30, 29, 28  };
byte nixie4[]={
//   0   1   2   3   4   5   6   7   8   9  
    35, 34, 33, 47, 46, 24, 39, 38, 37, 36  };
byte nixie5[]={
//   0   1   2   3   4   5   6   7   8   9  
    54, 52, 51, 44, 45, 43, 42, 41, 40, 55  };
byte nixie6[]={
//   0   1   2   3   4   5   6   7   8   9  
    60, 59, 58, 57, 50, 49, 48, 63, 62, 61  };

// ********************************************************************************************************
// ********************************************************************************************************
// ********************************************************************************************************


void setup() 
{
  Serial.begin(9600);
  main_rtc.begin();
  ds3231_rtc.begin();  
 
  pinMode(dataPin, OUTPUT);       // Nixie driver serial data input 
  digitalWrite(dataPin, LOW);    
    
  pinMode(clockPin, OUTPUT);       // Nixie driver data shift register clock input 
  digitalWrite(clockPin, LOW);         
  
  pinMode(latchPin, OUTPUT);       // Nixie driver output enable input
  digitalWrite(latchPin, LOW);

  // Turn off NPS module
  NixieTubes(OFF);
  
  delay(5000);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  delay(1000);
  
  // Invoking `addCallback` on the ArduinoCloud object allows you to subscribe
  // to any of the available events and decide which functions to call when they are fired.
  // doThisOnSync() will be called when the Clock syncs data with the cloud
  // doThisOnConnect() will be called when the Clock connects to the cloud
  // doThisOnDisconnect() will be called when the Clock disconnects from the cloud
  
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);

  // The following function allows you to obtain more information
  // related to the state of network and IoT Cloud connection and errors
  // the higher number the more granular information you’ll get.
  // The default is 0 (only errors).
  // Maximum is 4
  
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo(); 
  
  ds3231_rtc.begin();  

  DelayTime(1000);

  Serial.println(" ");
  Serial.println("#############################################################");
  Serial.println("------------------- IoT IN12 Nixie Clock --------------------");
  Serial.println("#############################################################");    
  Serial.println('\n');

  DelayTime(1000);
    
  Serial.println("Trying to connect with Arduino IoT Cloud...");
  
  // Wait for connection with Arduino IoT Cloud
  while (ArduinoCloud.connected() == 0) 
  {
    ArduinoCloud.update();
  }

  led.begin();                                    // Initialize NeoPixel led object
  led.show();                                     // Turn OFF all pixels ASAP
  led.setBrightness(255);                         // Set full brightness  

  // led.fill(red_color);
  led.fill(led.Color(255, 0, 0));         // Fill all LEDs with a color red
  led.show();                             // Update LEDs
  delay(1000);

  NixieTubes(ON);       // Turn ON nixie power supply module
}

void loop() 
{    
  DelayTime(1000);
  DisplayTime();
}

void DisplayTime()
{   
  DateTime now = ds3231_rtc.now();

  timeHour = now.hour();
  timeMinute = now.minute();
  timeSecond = now.second();

  // In such a written configuration of time display, there is no simple way 
  // to check the exact second when the time should be synchronized, 
  // so the time synchronization is set to 3:00 AM 
  // and to avoid multiple time synchronization at this time, 
  // after time synchronization, the next time synchronization condition check 
  // will be possible after 60 seconds
    
  if(timeHour == timeToSynchronizeTime && timeMinute == 00)
  {
    if(millis() - millis_start > 60000)
    {
      SynchronizeTime();
      millis_start = millis();
    }
  }

  if(status_nixie_clock == true)
  { 
    uint8_t timeFormat = hourFormat;
    
    // Check time format and adjust
    if(timeFormat == 12 && timeHour > 12) timeHour = timeHour - 12;
    if(timeFormat == 12 && timeHour == 0) timeHour = 12; 

    // Extract individual digits
    digit1  = (timeHour     / 10)  % 10;
    digit2  = (timeHour     / 1)   % 10;
    digit3  = (now.minute() / 10)  % 10;
    digit4  = (now.minute() / 1)   % 10;
    digit5  = (now.second() / 10)  % 10;
    digit6  = (now.second() / 1)   % 10; 

    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);     
/*
    Serial.print("Current Time: ");
    Serial.print(digit1); Serial.print(digit2);
    Serial.print(':');
    Serial.print(digit3); Serial.print(digit4);
    Serial.print(':');
    Serial.print(digit5); Serial.print(digit6);
    Serial.println();
*/
    // Activate "Slot Machine Effect" every 60 seconds
    if(now.second() == 0)
    {
      SlotMachine();      
    } 
  }     
}

void SlotMachine()
{
  for(int i = 0; i < 60; i++)
  {   
    if(i >= 0) digit6 = digit6 + 1;        
    if(digit6 >= 10) digit6 = 0;
        
    if(i >= 5) digit5 = digit5 + 1;  
    if(digit5 >= 10) digit5 = 0;  

    if(i >= 10) digit4 = digit4 + 1;  
    if(digit4 >= 10) digit4 = 0; 

    if(i >= 15) digit3 = digit3 + 1;  
    if(digit3 >= 10) digit3 = 0; 

    if(i >= 20) digit2 = digit2 + 1;  
    if(digit2 >= 10) digit2 = 0; 

    if(i >= 25) digit1 = digit1 + 1;  
    if(digit1 >= 10) digit1 = 0; 

    DelayTime(50);
    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);       
    }
}

void SynchronizeTime()
{
  do 
  {
    epochTime = WiFi.getTime();
    DelayTime(100);
    
    numberOfTries++;
  }
  while ((epochTime == 0) && (numberOfTries < maxTries));

  Serial.println(" ");
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  main_rtc.setEpoch(epochTime);

  timeHour = main_rtc.getHours() + timeZone;
  if(timeHour < 0) timeHour = 24 + timeHour;  
  if(timeHour > 23) timeHour = timeHour - 24;  

  timeMinute = main_rtc.getMinutes(); 
  timeSecond = main_rtc.getSeconds();

  // Update RTC DS3231 module
  ds3231_rtc.adjust(DateTime(0, 0, 0, timeHour, timeMinute, timeSecond));

  Serial.println(" ");
  Serial.println("#############################################################");
  Serial.println("--------------- Time has been Synchronized ------------------");
  Serial.println("#############################################################");
  Serial.println(" ");
}

void NixieDisplay(byte digit1, byte digit2, byte digit3, byte digit4, byte digit5, byte digit6)
{    
  SetBacklight();

  // Convert the desired numbers to the bit numbers for the nixieDisplayArray[]
  digit1 = nixie1[digit1];
  digit2 = nixie2[digit2];
  digit3 = nixie3[digit3];
  digit4 = nixie4[digit4];
  digit5 = nixie5[digit5];
  digit6 = nixie6[digit6];
    
  // Clear bit array except bit EN - 32 (Nixie Power Supply ON/OFF bit) 
  for (int i = 63; i >= 0; i--)
  {
    if(i != EN) nixieDisplayArray[i] = 0;      
  }
    
  // Set the bits corresponding to the nixie tubes cathodes
  nixieDisplayArray[digit1] = 1;
  nixieDisplayArray[digit2] = 1;
  nixieDisplayArray[digit3] = 1;
  nixieDisplayArray[digit4] = 1;
  nixieDisplayArray[digit5] = 1;
  nixieDisplayArray[digit6] = 1;    

  ShiftOutData();
}

void ShiftOutData()
{
  // Ground latchPin and hold low for as long as you are transmitting
  digitalWrite(latchPin, 0); 
  // Clear everything out just in case to
  // prepare shift register for bit shifting
  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);

  // Send bit array to the nixie drivers 
  for (int i = 63; i >= 0; i--)
  {
    // Sets the pin to HIGH or LOW depending on bit state
    digitalWrite(dataPin, nixieDisplayArray[i]);
    // Register shifts bits on upstroke of clock pin 
    digitalWrite(clockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(clockPin, 0);  
  }

  // Return the latch pin high to signal chip that it 
  // no longer needs to listen for information
  digitalWrite(latchPin, 1);
    
  // Stop shifting
  digitalWrite(clockPin, 0);     
}

void NixieTubes(boolean onoff)
{
  // Enable/Disable NPS Nixie Power Supply module
  // !onoff negation because "0" for NPS module turn on and "1" for turn off
  nixieDisplayArray[EN] = !onoff;
  ShiftOutData();
}

void SetBacklight(void)
{
  if(status_nixie_clock == true)
  {
    uint16_t new_hue_Value = 0;
    uint8_t new_sat_Value = 0;
    uint8_t new_bri_Value = 0;
  
    if(status_backlight == true)
    {
      new_hue_Value = current_hue_Value;
      new_sat_Value = current_sat_Value;
      new_bri_Value = current_bri_Value;
    }

    // Convert HSB to RGB
    // Declare a variable of the Color data type and define it using the HSB values of the color variable
    Color currentColor = Color(new_hue_Value, new_sat_Value, new_bri_Value);

    // Declare the variables to store the RGB values
    uint8_t RValue;
    uint8_t GValue;
    uint8_t BValue;

    // The variables will contain the RGB values after the function returns
    currentColor.getRGB(RValue, GValue, BValue);

    uint32_t new_backlight_Color = led.Color(RValue, GValue, BValue);
 
    // Fill all LEDs with a color
    led.setBrightness(255);
    led.fill(new_backlight_Color);
    led.show();
    Serial.println(new_backlight_Color);
  }
}

// Executed every time a new value is received from IoT Cloud.
void onBacklightChange()
{
  Serial.println("#############################################################");
  Serial.println("Inside: onBacklightChange");
  Serial.println("Setup backlight");
  Serial.println("#############################################################");

  current_hue_Value = backlight.getValue().hue;
  current_sat_Value = backlight.getValue().sat;
  current_bri_Value = backlight.getValue().bri;
  status_backlight = backlight.getSwitch();

Serial.println(current_hue_Value);
Serial.println(current_sat_Value);
Serial.println(current_bri_Value);
Serial.println(status_backlight);

}

void onNixieClockChange()
{
  Serial.println("#############################################################");
  Serial.println("Inside: onNixieClockChange");
  Serial.println("#############################################################");
  
  status_nixie_clock = nixieClock;
  
  if(status_nixie_clock == true)
  {
    // Turn ON Nixie Power Supply Module
    NixieTubes(ON);

    Serial.println("#############################################################");      
    Serial.println("IoT IN12 Nixie Clock Turn ON");
    Serial.println("#############################################################");
  }
  else
  { 
    // Turn OFF Nixie Power Supply Module
    NixieTubes(OFF);
    
    Serial.println("#############################################################");
    Serial.println("IoT IN12 Nixie Clock Turn OFF");
    Serial.println("#############################################################");

    led.clear();
    led.show(); 
    DelayTime(2000);
  }
}

// To minimize communication delays with the Arduino IoT Cloud 
// most of the DelayTime() functions have been replaced by DelayTime()
// with the ArduinoCloud.update() function call inside.
// The time needed to update the data from the Arduino IoT Cloud is 
// about 5-20ms, so the update function hidden inside the delay routine
// will not interfere with the program's operation.
void DelayTime(uint32_t wait)
{
  uint32_t millis_time_now = millis();
  while(millis() - millis_time_now < wait)
  {         
    ArduinoCloud.update();
  }  
}

void doThisOnSync()
{ 
  Serial.println("#############################################################");
  Serial.println("Inside: doThisOnSync");
  Serial.println("Satisfactory data synchronization");
  Serial.println("#############################################################");
}

void doThisOnConnect()
{
  Serial.println("#############################################################");
  Serial.println("Inside: doThisOnConnect");
  Serial.println("Connected to Arduino IoT Cloud");
  Serial.println("#############################################################");

  Serial.println(" ");
  SynchronizeTime();
}

void doThisOnDisconnect()
{  
  Serial.println("#############################################################");
  Serial.println("Inside: doThisOnDisconnect");
  Serial.println("No Time to Die");
  Serial.println("Check your System");
  Serial.println("#############################################################");  
}
