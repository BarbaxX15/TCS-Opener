
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

WiFiClient net;
PubSubClient client(net);

char wifi_ssid[] = "";
char wifi_pass[] = "";
char mqtt_server_ip[] = "";
char mqtt_port[] = "1883";
char mqtt_topic_bell[] = "home/bell";
char mqtt_topic_open[] = "home/open";
char mqtt_client_name[] = "TCS Opener";
char mqtt_user[] = "";
char mqtt_pass[] = "";
char mqtt_open_command[] = "open";

#define inputPin 12 // D6
#define outputPin 14 // D5
#define startBit 6
#define oneBit 4
#define zeroBit 2
#define opener_short 4352 // HEX: 1100 | BIN: 0001000100000000

volatile uint32_t CMD = 0;
volatile uint8_t lengthCMD = 0;
volatile bool cmdReady;

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

void connectToMqtt()
{
    Serial.print("MQTT server address: ");
    Serial.println(mqtt_server_ip);
    Serial.print("MQTT port: ");
    Serial.println(atoi(mqtt_port));
    Serial.print("Connecting to MQTT");
    client.setServer(mqtt_server_ip, atoi(mqtt_port));

    while (!client.connect(mqtt_client_name, mqtt_user, mqtt_pass))
    {
        Serial.print(".");
        delay(5000);
    }
    Serial.println("");
    Serial.println("Connected to MQTT.");
    client.setCallback(mqttCallback);
    client.subscribe(mqtt_topic_open);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    char payload_string[(sizeof payload) + 1];
    memcpy(payload_string, payload, sizeof payload);
    payload_string[sizeof payload] = 0; // Null termination.
    Serial.println(payload_string);
    if(strcmp(payload_string,mqtt_open_command)==0){
      Serial.println("MQTT open command received");
      sendProtocolHEX(opener_short);
    }
}

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
    connectToMqtt();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi();
    }
    if (!client.connected())
    {
        connectToMqtt();
    }
    client.loop();
    if (cmdReady)
    {
        Serial.print("Protocol received: ");
        printHEX(CMD);
        Serial.println("Publishing to MQTT.");
        cmdReady = 0;
        char byte_cmd[9];
        sprintf(byte_cmd, "%08x", CMD);
        client.publish(mqtt_topic_bell, byte_cmd);
    }
}
