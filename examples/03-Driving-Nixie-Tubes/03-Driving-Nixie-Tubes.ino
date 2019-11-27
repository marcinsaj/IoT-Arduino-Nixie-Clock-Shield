// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// Driving Nixie Tubes Example
//
// This example demonstrates how to display digits on nixie tubes.
// The control is carried out using eight HCT595 shift registers and high voltage transistors.
//
// Hardware:
// Arduino Uno/Nano/Micro 
// IoT Arduino Nixie Clock Shield
// 12V Power Supply

#include <FastLED.h>          // https://github.com/FastLED/FastLED

#define LED_PIN     A0        // Data pin that LEDs data will be written out over
#define NUM_LEDS    6         // LEDs using for RGB Calibration
#define BRIGHTNESS  80        // The best tested LEDs brightness 0-80, do not exceed 80
#define LED_TYPE    WS2812B   // Datasheet: http://bit.ly/LED-WS2812B
#define COLOR_ORDER GRB       // RGB, GRB, GBR etc.

CRGB leds[NUM_LEDS];          // Define the array of LEDs

#define dataPin    9          // HCT595 shift register serial data pin             
#define clockPin   8          // Clock pin
#define latchPin   7          // Latch pin

// Bit states for Nixie Power Supply module control and dot1 & dot2 state
#define ON         1          
#define OFF        0             

byte dot1 = 26;       // Dot cathode - IN-12B nixie tube no.3
byte dot2 = 53;       // IN-12B nixie tube no.5
byte EN   = 32;       // Enable bit number for NPS Nixie Power Supply module  

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


void setup() 
{  
    pinMode(dataPin, OUTPUT);        // Nixie driver serial data input 
    digitalWrite(dataPin, LOW);    
    
    pinMode(clockPin, OUTPUT);       // Nixie driver data shift register clock input 
    digitalWrite(clockPin, LOW);         
  
    pinMode(latchPin, OUTPUT);       // Nixie driver output enable input
    digitalWrite(latchPin, LOW);

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
    NixieTubes(ON);
}

void loop ()
{
    // Do simple counting
    for(int i = 0; i <= 9; i++)
    {
        // 6 digits
        NixieDisplay(i, i, i, i, i, i);
        
        // Dots
        SetDot(dot1, ON);
        SetDot(dot2, OFF);
        delay(500);  
        SetDot(dot1, OFF);
        SetDot(dot2, ON);
        delay(500);         
    }
}

void SetBacklightColor()
{
    for(int i = 0; i < 6; i++)
    {
        // https://github.com/FastLED/FastLED/wiki/Pixel-reference
        leds[i] = CRGB::Blue;   
    } 
    FastLED.show();
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
void NixieTubes(boolean onoff)
{
    // Enable/Disable NPS Nixie Power Supply module
    // Negation because "0" for NPS module turn on and "1" for turn off
    nixieDisplayArray[EN] = !onoff;
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

void SetDot(byte dot, boolean onoff)
{
    // Turn ON/OFF dot1 or dot2
    nixieDisplayArray[dot] = onoff;
    ShiftOutData();      
}
