/*
   Basic ESP8266 MQTT example

   This sketch demonstrates the capabilities of the pubsub library in combination
   with the ESP8266 board/library.

   It connects to an MQTT server then:
   - publishes "hello world" to the topic "outTopic" every two seconds
   - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
   - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

   It will reconnect to the server if the connection is lost using a blocking
   reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
   achieve the same result without blocking the main loop.

   To install the ESP8266 board, (using Arduino 1.6.4+):
   - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
   - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
   - Select your ESP8266 in "Tools -> Board"

 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);

RCSwitch mySwitch = RCSwitch();

// forward declaration
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);

void setup() {
        Serial.begin(115200);
        setup_wifi();
        client.setServer(MQTT_SERVER, 1883);
        client.setCallback(callback);

        mySwitch.enableTransmit(PIN_TX);
        mySwitch.enableReceive(PIN_RX); // Receiver on interrupt
}

void setup_wifi() {
        delay(10);
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(WIFI_SSID);

        WiFi.begin(WIFI_SSID, WIFI_PASS);

        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
        StaticJsonBuffer<200> jsonBuffer;

        JsonObject& root = jsonBuffer.parseObject(payload);

        int code = root["code"];
        int protocol = root["protocol"];
        int pulseLength = root["pulseLength"];

        mySwitch.setProtocol(protocol);
        mySwitch.setPulseLength(pulseLength);
        mySwitch.send(code, 24);
}

void reconnect() {
        // Loop until we're reconnected
        while (!client.connected()) {
                Serial.print("Attempting MQTT connection...");
                // Attempt to connect
                if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASS)) {
                        Serial.println("connected");
                        // ... and resubscribe
                        client.subscribe(MQTT_PATH "tx/");
                } else {
                        Serial.print("failed, rc=");
                        Serial.print(client.state());
                        Serial.println(" try again in 5 seconds");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}
void loop() {
        if (!client.connected()) {
                reconnect();
        }
        client.loop();

        if (mySwitch.available()) {
                int value = mySwitch.getReceivedValue();

                if (value == 0) {
                        Serial.print("Unknown encoding");
                } else {
                        StaticJsonBuffer<200> jsonBuffer;
                        JsonObject& root = jsonBuffer.createObject();

                        root["code"] = mySwitch.getReceivedValue();
                        root["protocol"] = mySwitch.getReceivedProtocol();

                        char jsonChar[100];
                        root.printTo(jsonChar);

                        client.publish(MQTT_PATH "rx/", jsonChar);
                }
                mySwitch.resetAvailable();
        }
}
