// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// Classic Nixie Clock Example
//
// This example demonstrates how to set the RTC time, read time from RTC and display on nixie tubes.
// The user has the choice of 12 or 24 hour display format.
// Serial monitor is required to display basic options.
// DS3231 RTC datasheet: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
//
// Hardware:
// Arduino Uno/Nano/Micro 
// IoT Arduino Nixie Clock Shield
// 12V Power Supply

#include <FastLED.h>              // https://github.com/FastLED/FastLED
#include <RTClib.h>               // https://github.com/adafruit/RTClib

// Choose Time Format
#define HH          12            // 12 Hour Clock or 24 Hour Clock

#define LED_PIN     A0            // Data pin that LEDs data will be written out over
#define NUM_LEDS    6             // LEDs using for RGB Calibration
#define BRIGHTNESS  80            // The best tested LEDs brightness 0-80, do not exceed 80
#define LED_TYPE    WS2812B       // Datasheet: http://bit.ly/LED-WS2812B
#define COLOR_ORDER GRB           // RGB, GRB, GBR etc.
#define UPDATES_PER_SECOND 10     // Updates per second 1-100

#define BACKLIGHT   1             // Motion color = 1, Static color = 0
CRGB COLOR = CRGB(0,0,255);       // Choose your own static color
         
CRGBPalette16 currentPalette;     // https://github.com/FastLED/FastLED/wiki/Gradient-color-palettes     
TBlendType    currentBlending;

static uint8_t colorIndex = 0;

CRGB leds[NUM_LEDS];              // Define the array of LEDs

#define dataPin     9             // HCT595 shift register serial data pin             
#define clockPin    8             // Clock pin
#define latchPin    7             // Latch pin

// Bit states for Nixie Power Supply module control and dot1 & dot2 state
#define ON          1          
#define OFF         0 

RTC_DS3231 rtc;

byte dot1 = 26;       // Dot cathode - IN-12B nixie tube no.3
byte dot2 = 53;       // IN-12B nixie tube no.5
byte EN   = 32;       // Enable bit number for NPS Nixie Power Supply module  

// Serial monitor state
boolean serialState = 0;

// Millis delay time variable 
unsigned long previous_millis = 0;

// Bit array for 6 nixie tubes, dot1, dot2, EN
boolean nixieDisplayArray[64];

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
    Serial.println("------------------------Nixie Clock -------------------------");
    Serial.println("---------------- If you want to set new Time ----------------");
    Serial.println("--------------- press ENTER within 5 seconds ----------------");

    // Millis time start
    unsigned long millis_time_now = millis();
    unsigned long millis_time_now_2 = millis();
    
    // Wait 5 seconds
    while((millis() < millis_time_now + 5000))
    {    
        // Print progress bar      
        if (millis() - millis_time_now_2 > 80)
        {
            Serial.print("#");
            millis_time_now_2 = millis();    
        }

        // Set serialState flag if time settings have been selected 
        if(Serial.available() > 0) 
        {            
            serialState = 1;
            break;  
        }
    }

    Serial.println('\n');
    
    // Clear serial buffer
    while(Serial.available())
    Serial.read();

    // Turn on NPS module
    NixieTubes(ON);
}

void loop ()
{
    SetBacklightColor();
    
    // Send time to the RTC if the time settings have been selected
    if(serialState == 1)
    {
        SetNewTime();
        serialState = 0;             
    }

    // Millis time start
    unsigned long current_millis = millis();

    // Wait 1 second
    if(current_millis - previous_millis >= 1000)
    {
        previous_millis = current_millis;      

        // Get time from RTC and display on nixie tubes
        DisplayTime();
    }
}

void SetNewTime()
{  
    Serial.println("----- Enter the TIME without spaces in the HHMMSS format -----");
    Serial.println("- and press enter when you are ready to send data to the RTC -");
    Serial.println('\n');

    // Clear serial buffer
    while(Serial.available())
    Serial.read();
    
    // Wait for the values
    while (!Serial.available()) {}                      

    // Read time as 32 bit value - 6 digits
    int32_t hhmmss_time = Serial.parseInt();

    // Extract hours, minutes and seconds
    byte Hours   = (hhmmss_time / 10000) % 100;
    byte Minutes = (hhmmss_time / 100)   % 100;
    byte Seconds = (hhmmss_time / 1)     % 100;    

    // First three "0" values represent the date - will not be used
    rtc.adjust(DateTime( 0, 0, 0, Hours, Minutes, Seconds));
}

void DisplayTime()
{
    DateTime now = rtc.now();
 
    byte timeHour = now.hour();
    byte timeFormat = HH;
    
    // Check time format and adjust
    if(timeFormat == 12 && timeHour > 12) timeHour = timeHour - 12;
    if(timeFormat == 12 && timeHour == 0) timeHour = 12;   

    // Extract individual digits
    byte digit1  = (timeHour     / 10)  % 10;
    byte digit2  = (timeHour     / 1)   % 10;
    byte digit3  = (now.minute() / 10)  % 10;
    byte digit4  = (now.minute() / 1)   % 10;
    byte digit5  = (now.second() / 10)  % 10;
    byte digit6  = (now.second() / 1)   % 10; 

    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);     

    Serial.print("Current Time: ");
    Serial.print(digit1); Serial.print(digit2);
    Serial.print(':');
    Serial.print(digit3); Serial.print(digit4);
    Serial.print(':');
    Serial.print(digit5); Serial.print(digit6);
    Serial.println();
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
