# TCS-Opener
App collection for Android Notifications from TCS Intercom

The collection consists of:
 - an ESP8266 board program
 - an Android app

The intended use is as follows:
For ESP:
 - Wire up the ESP8266 board according to the schematics (in ESP8266 directory)
 - Create a Firebase project on Google Firebase, download the firebase.json
 - Install required Arduino libraries
 - Edit program config and upload the program to the ESP8266
For Android App:
 - Put your firebase google-services.json from Firebase into the app/src/ directory
 - Define the variables in gradle.properties file:
    TCS_SERIAL_NUMBER=""
    MQTT_USERNAME=""
    MQTT_PASSWORD=""
    MQTT_DEFAULT_BROKER_IP="IP:1883"
 - Build and install the app


Functionality:
When receiving a doorbell protocol from the TCS bus, the ESP8266 sends a Firebase Message.
The Android app listens for the message and shows a notification.

For opening the door, the Android app sends a MQTT message to the MQTT broker on the ESP8266.
The ESP listens for the message "open" and then sends the OPEN-protocol to the bus.


Based on https://github.com/atc1441/TCSintercomArduino and https://github.com/Syralist/tcs-monitor