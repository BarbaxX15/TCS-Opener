
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <PicoMQTT.h>
#include <FirebaseESP8266.h>

//----------------CONFIG------------------//
#define wifi_ssid "SSID"
#define wifi_pass "WIFIPASS"

#define mqtt_topic_open "home/open" //topic for Android App (default)
#define mqtt_user "USER" //for auth by Android App
#define mqtt_pass "PASS" //for auth by Android App
#define mqtt_open_command "open" //sent by Android App (default)

//upload program to ESP and read SN from Serial Monitor after pressing doorbell (0XXXXXYY or 1XXXXXYY,
//where XXXXX is your SN)
#define TCS_SERIAL "XXXXX"

#define FIREBASE_PROJECT_ID "FIREBASE_PROJECT_ID" // retrieve from firebase.json
#define FIREBASE_CLIENT_EMAIL "FIREBASE_CLIENT_EMAIL" // retrieve from firebase.json
#define FIREBASE_MESSAGE_TOPIC "doorbell" //topic received by Android App
// retrieve from firebase.json
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY----------END PRIVATE KEY-----\n";
//--------------END CONFIG----------------//


// TCS
#define inputPin 12 // D6
#define outputPin 14 // D5
#define startBit 6
#define oneBit 4
#define zeroBit 2
#define opener_short 4352 // HEX: 1100 | BIN: 0001000100000000

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

volatile uint32_t CMD = 0;
volatile uint8_t lengthCMD = 0;
volatile bool cmdReady;

class MQTT: public PicoMQTT::Server {
    protected:
        PicoMQTT::ConnectReturnCode auth(const char * client_id, const char * username, const char * password) override {
            // only accept client IDs which are 3 chars or longer
            if (String(client_id).length() < 3) {    // client_id is never NULL
                return PicoMQTT::CRC_IDENTIFIER_REJECTED;
            }

            // only accept connections if username and password are provided
            if (!username || !password) {  // username and password can be NULL
                // no username or password supplied
                return PicoMQTT::CRC_NOT_AUTHORIZED;
            }

            // accept two user/password combinations
            if (String(username) == mqtt_user && String(password) == mqtt_pass) {
                return PicoMQTT::CRC_ACCEPTED;
                Serial.println("Client connected.");
            }

            // reject all other credentials
            return PicoMQTT::CRC_BAD_USERNAME_OR_PASSWORD;
        }
}mqtt;

// Network functions
void sendFirebaseMessage(const char *payload){
    FCM_HTTPv1_JSON_Message msg;
    msg.topic = FIREBASE_MESSAGE_TOPIC;

    FirebaseJson message;
    message.add("id", payload);
    msg.data = message.raw();

    msg.android.priority = "high";

    if (Firebase.FCM.send(&fbdo, &msg)) // send message to recipient
        Serial.println("Sent.");
    else
        Serial.println(fbdo.errorReason());
}

void setupFirebase(){
    config.service_account.data.client_email = FIREBASE_CLIENT_EMAIL;
    config.service_account.data.project_id = FIREBASE_PROJECT_ID;
    config.service_account.data.private_key = PRIVATE_KEY;
    Firebase.reconnectNetwork(false);
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
    Firebase.begin(&config, &auth);
}

void setupMqttBroker(){
    mqtt.subscribe(mqtt_topic_open, [](const char * topic, const char * payload) {
        Serial.print("MQTT payload incoming: ");
        Serial.println(payload);
        if(strcmp(payload,mqtt_open_command)==0){
            Serial.println("MQTT open command received.");
            sendProtocolHEX(opener_short);
        }
    });
    mqtt.begin();
    Serial.println("MQTT broker started on port 1883.");
}

void connectToWiFi()
{
    WiFi.begin(wifi_ssid, wifi_pass);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
         delay(1000);
    }
    Serial.println("");
    Serial.println("Connected to WiFi.");
    Serial.print("DHCP IP Address: ");
    Serial.println(WiFi.localIP());
}


// TCS functions

void ICACHE_RAM_ATTR analyzeCMD()
{
    static uint32_t curCMD;
    static uint32_t usLast;
    static byte curCRC;
    static byte calCRC;
    static byte curLength;
    static byte cmdIntReady;
    static byte curPos;
    uint32_t usNow = micros();
    uint32_t timeInUS = usNow - usLast;
    usLast = usNow;
    byte curBit = 4;
    if (timeInUS >= 1000 && timeInUS <= 2999)
    {
        curBit = 0;
    }
    else if (timeInUS >= 3000 && timeInUS <= 4999)
    {
        curBit = 1;
    }
    else if (timeInUS >= 5000 && timeInUS <= 6999)
    {
        curBit = 2;
    }
    else if (timeInUS >= 7000 && timeInUS <= 24000)
    {
        curBit = 3;
        curPos = 0;
    }
    if (curPos == 0)
    {
        if (curBit == 2)
        {
            curPos++;
        }
        curCMD = 0;
        curCRC = 0;
        calCRC = 1;
        curLength = 0;
    }
    else if (curBit == 0 || curBit == 1)
    {
        if (curPos == 1)
        {
            curLength = curBit;
            curPos++;
        }
        else if (curPos >= 2 && curPos <= 17)
        {
            if (curBit)
                bitSet(curCMD, (curLength ? 33 : 17) - curPos);
            calCRC ^= curBit;
            curPos++;
        }
        else if (curPos == 18)
        {
            if (curLength)
            {
                if (curBit)
                    bitSet(curCMD, 33 - curPos);
                calCRC ^= curBit;
                curPos++;
            }
            else
            {
                curCRC = curBit;
                cmdIntReady = 1;
            }
        }
        else if (curPos >= 19 && curPos <= 33)
        {
            if (curBit)
                bitSet(curCMD, 33 - curPos);
            calCRC ^= curBit;
            curPos++;
        }
        else if (curPos == 34)
        {
            curCRC = curBit;
            cmdIntReady = 1;
        }
    }
    else
    {
        curPos = 0;
    }
    if (cmdIntReady)
    {
        cmdIntReady = 0;
        if (curCRC == calCRC)
        {
            cmdReady = 1;
            lengthCMD = curLength;
            CMD = curCMD;
        }
        curCMD = 0;
        curPos = 0;
    }
}

void sendProtocolHEX(uint32_t protocol) {
  int length = 16;
  byte checksm = 1;
  byte firstBit = 0;
  if (protocol > 0xFFFF) {
    length = 32;
    firstBit = 1;
  }
  digitalWrite(outputPin, HIGH);
  delay(startBit);
  digitalWrite(outputPin, !digitalRead(outputPin));
  delay(firstBit ? oneBit : zeroBit);
  int curBit = 0;
  for (byte i = length; i > 0; i--) {
    curBit = bitRead(protocol, i - 1);
    digitalWrite(outputPin, !digitalRead(outputPin));
    delay(curBit ? oneBit : zeroBit);
    checksm ^= curBit;
  }
  digitalWrite(outputPin, !digitalRead(outputPin));
  delay(checksm ? oneBit : zeroBit);
  digitalWrite(outputPin, LOW);
}

void printHEX(uint32_t DATA)
{
    uint8_t numChars = lengthCMD ? 8 : 4;
    uint32_t mask = 0x0000000F;
    mask = mask << 4 * (numChars - 1);
    for (uint32_t i = numChars; i > 0; --i)
    {
        Serial.print(((DATA & mask) >> (i - 1) * 4), HEX);
        mask = mask >> 4;
    }
    Serial.println("");
}


void setup()
{
    Serial.begin(115200);
    Serial.println("Starting up.");
    pinMode(inputPin, INPUT_PULLUP);
    pinMode(outputPin, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(inputPin), analyzeCMD, CHANGE);
    connectToWiFi();
    setupMqttBroker();
    setupFirebase();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }

    mqtt.loop();

    if (cmdReady)
    {
        Serial.print("Protocol received: ");
        printHEX(CMD);
        cmdReady = 0;
        if(Firebase.ready()){
            char byte_cmd[9];
            sprintf(byte_cmd, "%08x", CMD);
            if(strstr(byte_cmd,TCS_SERIAL)!=NULL){
                Serial.println("Publishing to Firebase.");
                sendFirebaseMessage(byte_cmd);
            }
        }
    }
}
