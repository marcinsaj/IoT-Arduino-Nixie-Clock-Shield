// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// Color Palette Example
// Original code can be found here: 
// https://github.com/FastLED/FastLED/blob/master/examples/ColorPalette/ColorPalette.ino
//
// This example shows a few ways to set up colors with FastLED library.
//
// FastLED provides a few pre-configured color palettes, and makes it
// very easy to make up your own color schemes with palettes.
//
// Hardware:
// Arduino Uno/Nano/Micro 
// IoT Arduino Nixie Clock Shield
// 12V Power Supply

#include <FastLED.h>              // https://github.com/FastLED/FastLED

#define LED_PIN     A0            // Data pin that LEDs data will be written out over
#define NUM_LEDS    6             // All LEDs on shield 
#define BRIGHTNESS  80            // The best tested LEDs brightness 20-60
#define LED_TYPE    WS2812B       // Datasheet: http://bit.ly/LED-WS2812B
#define COLOR_ORDER GRB           // For color ordering use this sketch: http://bit.ly/RGBCalibrate   
#define UPDATES_PER_SECOND 10     // Updates per second 1-100

CRGB leds[NUM_LEDS];              // Define the array of LEDs

CRGBPalette16 currentPalette;     // https://github.com/FastLED/FastLED/wiki/Gradient-color-palettes
TBlendType    currentBlending;

static uint8_t colorIndex = 0;


void setup() 
{
    // Important power-up safety delay
    delay(3000); 

    // Limit my draw to 150mA at 5V of power draw
    FastLED.setMaxPowerInVoltsAndMilliamps(5,150);
    
    // Initialize LEDs
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    StaticColor();
    delay(5000);
    
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
}

void loop()
{       
    MotionColor(); 
}

void MotionColor()
{
    uint8_t brightness = 255;
    colorIndex = colorIndex + 1;
    
    for( int i = 0; i < NUM_LEDS; i++) 
    {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    }
    
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void StaticColor()
{
    // https://www.rapidtables.com/web/color/RGB_Color.html
    leds[0] = CRGB(255,0,0);    // RGB Red Color
    leds[1] = CRGB(0,255,0);    // RGB Green Color
    leds[2] = CRGB(0,0,255);    // RGB Blue Color 

    // https://github.com/FastLED/FastLED/wiki/Pixel-reference   
    leds[3] = CRGB::Red;
    leds[4] = CRGB::Green;
    leds[5] = CRGB::Blue;

    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}
