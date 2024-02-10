# TCS-Opener
App collection for Android Notifications from TCS Intercom

The collection consists of:
 - a local MQTT Broker (incl. client)
 - an ESP8266 board
 - an Android app

The intended use is as follows:
 - set up a local MQTT Broker on a NAS or a home server (Mosquitto)
 - wire up the ESP8266 board according to the schematics (in ESP8266 directory)
 - upload the program to the ESP8266
 - open Arduino IDE's Serial Monitor
 - ring the doorbell
 - check for your TCS Serial number in the Serial monitor (0XXXXXYY or 1XXXXXYY where XXXXX is SN)
 - install the node package in the MQTT Broker directory
 - create a Firebase project on Google Firebase, download the firebase.json and put it in the
   "MQTT Broker" directory
 - edit opener.js and enter your MQTT Broker credentials and your TCS serial number
 - run the opener.js with node
 - put your firebase google-services.json into the app/src/ directory
 - define the variables in gradle.properties file:
    TCS_SERIAL_NUMBER=""
    MQTT_USERNAME=""
    MQTT_PASSWORD=""
    MQTT_DEFAULT_BROKER_IP=""
 - build and install the app

When receiving a protocol from the TCS bus, the ESP8266 sends a MQTT message to the MQTT broker and
the MQTT client (which also could reside on the broker machine) relays the message to Firebase.
Then the Android app listens for the message and shows a notification.

For opening the door, the Android app sends a MQTT message to the MQTT broker and the ESP8266
listens for the message "open" and then sends the OPEN-protocol to the bus.




Based on https://github.com/atc1441/TCSintercomArduino and https://github.com/Syralist/tcs-monitor