Arduino mobile app
   - Android - https://bit.ly/arduino-android-app
   - iOS - https://bit.ly/arduino-ios-app

If you decide to use the Arduino Cloud Code Editor then use only this code: <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-IN12-Nixie-Clock.ino">IoT-IN12-Nixie-Clock.ino</a> and follow this <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-Arduino-Cloud-Setup.md">instructions</a>.
<br/>Optional Alexa setup can be found <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-Arduino-Cloud-Alexa-Setup.md">here</a>.

********************************************************  

If you use Arduino IDE offline software then you should download folowing files into one folder "IoT-IN12-Nixie-Clock":
   - <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-IN12-Nixie-Clock.ino">IoT-IN12-Nixie-Clock.ino</a>
   - <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/arduino-secrets.h">arduino-secrets.h</a>
   - t<a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/thingProperties.h">thingProperties.h</a>
   - 
Open "arduino-secrets.h" and "thingProperties.h" and edit the required informations:
   - define SECRET_SSID "type here your wifi network name"
   - define SECRET_PASS "type here your wifi password"
   - const char THING_ID[] = "place-here-id-number-of-your-thing-from-iot-arduino-cloud";
Upload sketch.
