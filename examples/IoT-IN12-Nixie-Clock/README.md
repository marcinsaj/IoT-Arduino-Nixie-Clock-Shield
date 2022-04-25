If you decide to use the Arduino Cloud Code Editor then use only this code: <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-IN12-Nixie-Clock.ino">IoT-IN12-Nixie-Clock.ino</a> and follow this <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-Arduino-Cloud-Setup.md">instructions</a>.
<br/>Optional Alexa setup can be found <a target="_blank" href="https://github.com/marcinsaj/IoT-Arduino-Nixie-Clock-Shield/blob/master/examples/IoT-IN12-Nixie-Clock/IoT-Arduino-Cloud-Alexa-Setup.md">here</a>.







1. Use this guide <a target="_blank" href="https://www.hackster.io/MarcinSaj/how-to-connect-one-nixie-clock-to-the-arduino-iot-cloud-e85081">"How to Connect ONC to the Arduino IoT Cloud"</a> and connect your clock to the Arduino IoT Cloud
2. Install Arduino app on your smartphone
   - Android - https://bit.ly/arduino-android-app
   - iOS - https://bit.ly/arduino-ios-app
3. Open this guide <a target="_blank" href="https://www.hackster.io/MarcinSaj/iot-one-nixie-clock-arduino-iot-cloud-alexa-control-85be50">"How to Control IoT ONE Nixie Clock via Arduino IoT Cloud or Alexa voice Control"</a> and follow step by step.
4. If you decide to use the Arduino Cloud Code Editor then use only this code "IoT-ONC-Backlight-Control.ino" <br/>
If you use Arduino IDE offline software then you should download folowing files into one folder "IoT-ONC-Backlight-Control":
   - IoT-ONC-Backlight-Control.ino
   - arduino-secrets.h
   - thingProperties.h
5. (offline) Open "arduino-secrets.h" and "thingProperties.h" and edit the required informations:
   - define SECRET_SSID "type here your wifi network name"
   - define SECRET_PASS "type here your wifi password"
   - const char THING_ID[] = "place-here-id-number-of-your-thing-from-iot-arduino-cloud";
6. Upload sketch.
<br/><br/>***ONC - ONE Nixie Clock
<p align="center"><img src="https://github.com/marcinsaj/ONE-Nixie-Clock/blob/main/extras/iot-one-nixie-clock.gif"></p>
