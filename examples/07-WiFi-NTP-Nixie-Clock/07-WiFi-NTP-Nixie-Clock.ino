/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
char ssid[] = "name wifi network";        // Your network SSID (name)
char pass[] = "wifi password";            // Your network password

const int timeZone = 2;                   // Change this to adapt it to your time zone 
                                          // https://en.wikipedia.org/wiki/List_of_time_zones_by_country

// Choose Time Format
#define HH          24                    // 12 Hour Clock or 24 Hour Clock                                  
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/


// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// NTP Nixie Clock with Slot Machine Effect
// This example demonstrates how to connect clock to WiFi and 
// synchronizing (ones per day) RTC DS3231 time module with NTP time server 
// 
// Serial monitor is required to debug synchronization
//
// Hardware:
// WiFi signal
// IN12 Nixie Clock - https://nixietester.com/project/iot-nixie-clock-shield-for-arduino
// Arduino Nano IoT 33 - https://store.arduino.cc/arduino-nano-33-iot
// Nixie Power Supply Module
// Nixie clock require 12V, 1A power supply

#include <WiFiNINA.h>                     // https://github.com/arduino-libraries/WiFiNINA
#include <WiFiUdp.h>
#include <RTCZero.h> 
#include <FastLED.h>                      // https://github.com/FastLED/FastLED
#include <RTClib.h>                       // https://github.com/adafruit/RTClib

int keyIndex = 0;                         // Your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

unsigned long epochTime;
int numberOfTries = 0, maxTries = 30;

#define timeToSynchronizeTime 3           // Each day at 3AM, the RTC DS3231 time will be synchronize with WIFI time
boolean timeToSynchronizeTimeFlag = 0;

byte timeHour = 0;
byte timeMinute = 0;
byte timeSecond = 0;

RTC_DS3231 rtcModule;                     // RTC module library declaration
RTCZero rtc;                              // Arduino Nano Every and IoT 33 have built in RTC 
                                          // this RTC is used by the WiFiNINA.h library
                                          // But for timekeeping we will use DS3231 RTC module because is more accurate

#define LED_PIN     A0                    // Data pin that LEDs data will be written out over
#define NUM_LEDS    6                     // LEDs using for RGB Calibration
#define BRIGHTNESS  80                    // The best tested LEDs brightness 0-80, do not exceed 80
#define LED_TYPE    WS2812B               // Datasheet: http://bit.ly/LED-WS2812B
#define COLOR_ORDER GRB                   // RGB, GRB, GBR etc.
#define UPDATES_PER_SECOND 10             // Updates per second 1-100


/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
#define BACKLIGHT   1                     // Motion color = 1, Static color = 0
CRGB COLOR = CRGB(0,0,255);               // Choose your own static color
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
         
CRGBPalette16 currentPalette;             // https://github.com/FastLED/FastLED/wiki/Gradient-color-palettes     
TBlendType    currentBlending;

static uint8_t colorIndex = 0;

CRGB leds[NUM_LEDS];                      // Define the array of LEDs

#define dataPin     9                     // HCT595 shift register serial data pin             
#define clockPin    8                     // Clock pin
#define latchPin    7                     // Latch pin

// Serial monitor state
boolean serialState = 0;

// Millis delay time variable 
unsigned long previous_millis = 0;
unsigned long current_millis = 0;

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
  rtc.begin();
  rtcModule.begin();  
    
  pinMode(dataPin, OUTPUT);       // Nixie driver serial data input 
  digitalWrite(dataPin, LOW);    
    
  pinMode(clockPin, OUTPUT);       // Nixie driver data shift register clock input 
  digitalWrite(clockPin, LOW);         
  
  pinMode(latchPin, OUTPUT);       // Nixie driver output enable input
  digitalWrite(latchPin, LOW);

  // Turn off NPS module
  NixieTubes(OFF);    
    
  // Important power-up safety delay for LEDs
  delay(3000); 

  // Limit draw to 150mA at 5V of power draw
  FastLED.setMaxPowerInVoltsAndMilliamps(5,150);
    
  // Initialize LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // Brightness settings
  FastLED.setBrightness(BRIGHTNESS);

  delay(1000);
  SetBacklightColor();

  Serial.println("#############################################################");
  Serial.println("------------------- IN12 NTP Nixie Clock --------------------");
  Serial.println("#############################################################");
  Serial.println();

  SynchronizeTimeWiFi();        // Try to connect to WiFi and synchronize time

  // Turn on NPS module
  NixieTubes(ON);
}

void loop() 
{    
  SetBacklightColor();
  
  // Check if it's time to synchronize the time  
  if(timeToSynchronizeTimeFlag == 1)
  {
    SynchronizeTimeWiFi();    
  }
  
  // Millis time start
  current_millis = millis();

  // Wait 1 second
  if(current_millis - previous_millis >= 1000)
  {
    // Get time from RTC and display on nixie tubes
    DisplayTime();
    previous_millis = current_millis;      
  }
}

void DisplayTime()
{
  DateTime now = rtcModule.now();
  timeHour = now.hour();
  byte timeFormat = HH;
    
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

  Serial.print("Current Time: ");
  Serial.print(digit1); Serial.print(digit2);
  Serial.print(':');
  Serial.print(digit3); Serial.print(digit4);
  Serial.print(':');
  Serial.print(digit5); Serial.print(digit6);
  Serial.println();

  // Activate "Slot Machine Effect" every 60 seconds
  if(now.second() == 0)
  {
        SlotMachine();      
  }
}

void NixieDisplay(byte digit1, byte digit2, byte digit3, byte digit4, byte digit5, byte digit6)
{    
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

    delay(50);
    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);       
    }
}

void NixieTubes(boolean onoff)
{
  // Enable/Disable NPS Nixie Power Supply module
  // !onoff negation because "0" for NPS module turn on and "1" for turn off
  nixieDisplayArray[EN] = !onoff;
  ShiftOutData();
}

void SetBacklightColor()
{
  if (BACKLIGHT == 1) // Motion color
  {
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
        
    uint8_t brightness = 255;
    colorIndex = colorIndex + 1;    
    
    for( int i = 0; i < NUM_LEDS; i++) 
    {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    }
  } 
  else  // Static color
  {
    for(int i = 0; i < 6; i++)
    {
      leds[i] = COLOR;
    }   
  }
    
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void SynchronizeTimeWiFi()
{
  int i  = 0;

  // Try connecting to the WiFi up to 5 times
  while ((status != WL_CONNECTED) && i < 5) 
  {
    Serial.println();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // Wait 10 seconds for connection:
    // Print progress bar      
    int progressBar = 0;
    
    while(progressBar <= 60)
    {
      Serial.print("#");
      delay(165);
      progressBar++;    
    }
    
    i++;
  }
  
  Serial.println();

  if(status == WL_CONNECTED)
  {
    Serial.println("You are connected to WiFi!");  
    
    // Attempt to WiFi time synchronization
    GetTimeWiFi();
  }
  else if (status != WL_CONNECTED)
  {
    Serial.println("The WiFi connection failed");
    Serial.println("Check your WIFI connection and credentials");
    Serial.println("Next attempt to connect in 24 hours");  
  }
  
  timeToSynchronizeTimeFlag = 0;
  PrintWiFiStatus();  
}

void GetTimeWiFi()
{
  do 
  {
    epochTime = WiFi.getTime();
    numberOfTries++;
  }
  while ((epochTime == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) 
  {
    Serial.println("Time synchronization is failed");
    Serial.println("Next attempt to synchronize time in 24 hours");
    Serial.println(); 
    delay(3000);
  }
  else 
  {
    Serial.println("Time synchronization succeeded!");
    Serial.println();
    rtc.setEpoch(epochTime);

    timeHour = rtc.getHours() + timeZone;
    
    if(timeHour < 0)
    {
      timeHour = 24 + timeHour;  
    }

    if(timeHour > 23)
    {
      timeHour = timeHour - 24;  
    }
    
    timeMinute = rtc.getMinutes();
    timeSecond = rtc.getSeconds();

    // Update RTC DS3231 module
    rtcModule.adjust(DateTime(0, 0, 0, timeHour, timeMinute, timeSecond));
  }  
}

void PrintWiFiStatus() 
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println();
}
