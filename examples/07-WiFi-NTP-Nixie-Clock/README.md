Change these lines:

char ssid[] = "name wifi network";        // Your network SSID (name)
char pass[] = "wifi password";            // Your network password

const int timeZone = 2;                   // Change this to adapt it to your time zone 
                                          // https://en.wikipedia.org/wiki/List_of_time_zones_by_country

// Choose Time Format
#define HH          24                    // 12 Hour Clock or 24 Hour Clock 

#define BACKLIGHT   1                     // Motion color = 1, Static color = 0
CRGB COLOR = CRGB(0,0,255);               // Choose your own static color
