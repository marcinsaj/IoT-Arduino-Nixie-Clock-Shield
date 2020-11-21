// IoT Arduino Nixie Clock Shield by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield
//
// Classic Nixie Thermometer & Hygrometer - Celsius or Fahrenheit
// with backlight and Slot Machine Effect every 10 seconds
//
// Users can choose units: Celsius or Fahrenheit
// & backlight effect (motion, static) and color
//
// Hardware:
// Arduino Uno/Nano/Micro 
// IoT Arduino Nixie Clock Shield
// 12V Power Supply
// DHT22 temperature and humidity sensor

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include "DHT.h"
#include <FastLED.h>              // https://github.com/FastLED/FastLED

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~ Choose your units ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
boolean tempUnits = 1;            // Fahrenheit = 1, Celsius = 0
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~ Chose your backlight ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
boolean backlightEffect = 1;      // Motion color = 1, Static color = 0
CRGB COLOR = CRGB(0,0,255);       // Choose your own color
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//***************************************************************************************************
//***** LED *****************************************************************************************
//***************************************************************************************************
#define LED_PIN     A0            // Data pin that LEDs data will be written out over
#define NUM_LEDS    6             // LEDs using for RGB Calibration
#define BRIGHTNESS  80            // The best tested LEDs brightness 0-80, do not exceed 80
#define LED_TYPE    WS2812B       // Datasheet: http://bit.ly/LED-WS2812B
#define COLOR_ORDER GRB           // RGB, GRB, GBR etc.
#define UPDATES_PER_SECOND 10     // Updates per second 1-100

CRGB leds[NUM_LEDS];              // Define the array of LEDs
CRGBPalette16 currentPalette;     // https://github.com/FastLED/FastLED/wiki/Gradient-color-palettes     
TBlendType    currentBlending;

static uint8_t colorIndex = 0;
//***************************************************************************************************

//***************************************************************************************************
//***** SENSOR **************************************************************************************
//***************************************************************************************************
#define DHTPIN      A1            // Digital pin connected to the DHT sensor
#define DHTTYPE     DHT22         // Sensor type
DHT dht(DHTPIN, DHTTYPE);         // Initialize DHT sensor

float tempDHT_C     = 0;          // Celsius temperature 
float tempDHT_F     = 0;          // Fahrenheit temperature  
float humDHT        = 0;          // Humidity

boolean tempHum     = 1;          // "1" - display temperature (default), "0" - display humidity
//***************************************************************************************************

//***************************************************************************************************
//***** SHIFT REGISTER ******************************************************************************
//***************************************************************************************************
#define dataPin       9           // HCT595 shift register serial data pin             
#define clockPin      8           // Clock pin
#define latchPin      7           // Latch pin
//***************************************************************************************************

//***************************************************************************************************
//***** NIXIE & REST ********************************************************************************
//***************************************************************************************************
// Bit array for 6 nixie tubes, dot1, dot2, EN
boolean nixieDisplayArray[64];

byte digit1, digit2, digit3, digit4, digit5, digit6;

// Cathodes assignment to the position in the 64 bit array
// Each cathode of nixie tubes is connected to the corresponding outputs of the shift register
// Bit numbers
byte nixie1[]={
//   0   1   2   3   4   5   6   7   8   9      - IN-12A/B  
     5,  4,  3,  2,  1,  0, 15, 14,  7,  6  };
byte nixie2[]={
//   0   1   2   3   4   5   6   7   8   9      - IN-12A/B 
    22, 21, 20, 13, 12, 11, 10,  9,  8, 23  };
byte nixie3[]={
//   0   1   2   3   4   5   6   7   8   9      - IN-12B nixie tube with dot  
    27, 25, 19, 18, 17, 16, 31, 30, 29, 28  };
byte nixie4[]={
//   0   1   2   3   4   5   6   7   8   9      - IN-12A/B   
    35, 34, 33, 47, 46, 24, 39, 38, 37, 36  };
byte nixie5[]={
//   μ   n   %   Π   k   M   m   +   -   P      - IN-15A
//   0   1   2   3   4   5   6   7   8   9  
    54, 52, 51, 44, 45, 43, 42, 41, 40, 55  };
byte nixie6[]={
//   W   A   Ω       S   V   H   Hz      F      - IN-15B
//   0   1   2   3   4   5   6   7   8   9  
    60, 59, 58, 57, 50, 49, 48, 63, 62, 61  };

// Temperature and humidity symbols (cathode numbers)
#define plusTEMP      7    // +
#define minusTEMP     8    // -
#define temp_F        9    // F
#define hum_percent   2    // %
#define hum_H         6    // H
      
// Bit states for Nixie Power Supply module control and dot1 & dot2 state
#define ON            1          
#define OFF           0  
#define turnOffTube   77        

byte dot1 = 26;                   // Dot cathode - IN-12B nixie tube no.3
byte dot2 = 53;                   // IN-12B nixie tube no.5
byte EN   = 32;                   // Enable bit number for NPS Nixie Power Supply module  

// Millis delay time variable 
unsigned long previous_millis = 0;

int  loopCount = 0;
//***************************************************************************************************
//***************************************************************************************************
//***************************************************************************************************

void setup() 
{  
    Serial.begin(9600);
    
    pinMode(dataPin, OUTPUT);       // Nixie driver serial data input 
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

    // DHT sensor start
    dht.begin();
}

void loop() 
{
    SetBacklightColor();
    
    // Millis time start
    unsigned long current_millis = millis();

    // Wait 5 second
    if(current_millis - previous_millis >= 5000)
    {
        loopCount++;  
            
        // Swap display values between temperature and humidity every 10 seconds
        // 5s (default delay between measurements) * loopCount(2) = 10s 
        if(loopCount >= 2)
        {
            loopCount = 0; 
            // Update flag value every 10s
            tempHum = !tempHum;     
            // Slot machine effect
            SlotMachine();     
        }
        
        previous_millis = current_millis;      

        // Get temperature and humidity 
        ReadSensor();

        // Display temperature or humidity
        if(tempHum == 1)
        {
            if(tempUnits == 1) DisplayFahrenheit();     // Fahrenheit
            if(tempUnits == 0) DisplayCelsius();        // Celsius
        }
        else
        {
            DisplayHumidity();    
        }
    }
}

void NixieDisplay(byte digit1, byte digit2, byte digit3, byte digit4, byte digit5, byte digit6)
{
    // Convert the desired numbers to the bit numbers for the nixieDisplayArray[]
    byte tube1 = nixie1[digit1];
    byte tube2 = nixie2[digit2];
    byte tube3 = nixie3[digit3];
    byte tube4 = nixie4[digit4];
    byte tube5 = nixie5[digit5];

    // IN15B - there are no cathodes number 3 and 8
    if(digit6 == 3 || digit6 == 8) digit6 = 6; 
    byte tube6 = nixie6[digit6];
    
    // Clear bit array except bit EN - 32 (Nixie Power Supply ON/OFF bit) 
    for (int i = 63; i >= 0; i--)
    {
        if(i != EN) nixieDisplayArray[i] = 0;      
    }
    
    // Set the bits corresponding to the nixie tubes cathodes
    if(digit1 != turnOffTube) nixieDisplayArray[tube1] = 1;
    if(digit2 != turnOffTube) nixieDisplayArray[tube2] = 1;
    if(digit3 != turnOffTube) nixieDisplayArray[tube3] = 1;
    if(digit4 != turnOffTube) nixieDisplayArray[tube4] = 1;
    if(digit5 != turnOffTube) nixieDisplayArray[tube5] = 1;
    if(digit6 != turnOffTube) nixieDisplayArray[tube6] = 1;    

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

void ReadSensor()
{
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    humDHT = dht.readHumidity();
    // Read temperature as Celsius (the default)
    tempDHT_C = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    tempDHT_F = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(humDHT) || isnan(tempDHT_C) || isnan(tempDHT_F)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    tempDHT_F = dht.computeHeatIndex(tempDHT_F, humDHT);
    // Compute heat index in Celsius (isFahreheit = false)
    tempDHT_C = dht.computeHeatIndex(tempDHT_C, humDHT, false);

    Serial.print("Temperature: ");
    Serial.print(tempDHT_C);
    Serial.print("°C; ");
    Serial.print(tempDHT_F);
    Serial.print("F");
    Serial.print("   Humidity: ");
    Serial.print(humDHT);  
    Serial.println("%");  
}

void DisplayCelsius()
{
    // We must get rid of the decimal point
    int currentTemp = tempDHT_C * 100;
       
    if(tempDHT_C >= 0) 
    {
        digit5 = plusTEMP;                    // Set "+" symbol if the Celsius temperature is > 0°C
    }
    else 
    {                                         // If the temperature is below 0°C
        currentTemp = -currentTemp;           // Convert a negative value to a positive for easier use  
        digit5 = minusTEMP;                   // Set "-" nixie tube symbol
    }   
    
    // Extract individual digits
    digit1 = (currentTemp / 1000) % 10;
    digit2 = (currentTemp / 100) % 10;
    digit3 = (currentTemp / 10) % 10;
    digit4 = turnOffTube;
    digit6 = turnOffTube; 
    
    // Display on nixie tubes
    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);
    SetDot(dot1, ON);
}

void DisplayFahrenheit()
{
    // We must get rid of the decimal point
    int currentTemp = tempDHT_F * 100;
    
    // Extract individual digits
    digit1 = (currentTemp / 1000) % 10;
    digit2 = (currentTemp / 100) % 10;
    digit3 = (currentTemp / 10) % 10;
    digit4 = turnOffTube;                   // Turn off nixie tube      
    digit5 = plusTEMP;                      // Set "+" nixie tube symbol                          
    digit6 = temp_F;                        // Set "F" nixie tube symbol
    
    // Display on nixie tubes
    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);
    //Turn on dot symbol
    SetDot(dot1, ON);        
}

void DisplayHumidity()
{
    // We must get rid of the decimal point
    int currentHum = humDHT * 100;
    
    // Extract individual digits
    digit1 = (currentHum / 1000) % 10;
    digit2 = (currentHum / 100) % 10;
    digit3 = (currentHum / 10) % 10;
    digit4 = turnOffTube;                   // Turn off nixie tube      
    digit5 = hum_percent;                   // Set "%" nixie tube symbol                          
    digit6 = hum_H;                         // Set "H" nixie tube symbol
    
    // Display on nixie tubes
    NixieDisplay(digit1, digit2, digit3, digit4, digit5, digit6);
    //Turn on dot symbol
    SetDot(dot1, ON);        
}

void SetBacklightColor()
{
    if (backlightEffect == 1) // Motion color
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

void SetDot(byte dot, boolean onoff)
{
    // Turn ON/OFF dot1 or dot2
    nixieDisplayArray[dot] = onoff;
    ShiftOutData();      
}
