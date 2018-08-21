//#include <Adafruit_SSD1306.h>

/*
    This sketch sends a message to a TCP server

*/

 #include <Wire.h>
 #include "SSD1306.h"
 #include "RobotoFonts.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>

/////////////////////////////////////////
// CHANGE TO MATCH YOUR CONFIGURATION  //
/////////////////////////////////////////
#define BROKER "192.168.0.1"
#define BROKER_PORT 1883
#define TOPIC "home/lolin/display"
#define MQTT_USER "user"
#define MQTT_PASS "admin"
#define CLIENT_ID "esp32_display"
#define WIFI_SSID "my_wifi"
#define WIFI_PASS "my_secret_password"
/////////////////////////////////////////

SSD1306  display(0x3c, 5, 4);
WiFiMulti WiFiMulti;
WiFiClient wclient;
PubSubClient clientMQTT(wclient);

String value = "R$ 0,00";
String percentage = "0.00%";
boolean shouldUpdateUI = false;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  StaticJsonBuffer<250> jsonBuffer; 
  char inData[length];

  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    inData[i] = (char)payload[i];
  }
  Serial.println();
  
  JsonObject& root = jsonBuffer.parseObject(inData);

  // Pull the value from key "VALUE" from the JSON message {"value": 1 , "someOtherValue" : 200}
  const char* val = root["value"];
  const char* perc = root["percentage"];
  value = String("R$ ") + String(val);
  percentage = String(perc) + String("%");
  shouldUpdateUI = true;

  Serial.println(val);
}

void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (clientMQTT.connect(CLIENT_ID)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      clientMQTT.publish("outTopic", "hello world");
      // ... and resubscribe
      clientMQTT.subscribe(TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  display.init();
//  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Roboto_Condensed_16);

  // We start by connecting to a WiFi network
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

  Serial.println();
  Serial.println();
  display.clear();
  display.drawString(0, 0, "Waiting for WiFi...");
  display.display();
  Serial.print("Waiting for WiFi...");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  display.clear();
  display.drawString(0, 0, "WiFi connected");
  display.drawString(0, 16, "IP address: ");
  display.drawString(0, 32, WiFi.localIP().toString());
  display.display();
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(3000);

  clientMQTT.setServer(BROKER, BROKER_PORT);
  clientMQTT.setCallback(callback);
  display.clear();
  display.drawString(0, 0, "Connecting to broker...");
  display.display();
  reconnect();
  display.drawString(0, 16, "Connected!");
  display.drawString(0, 32, "Waiting for messages...");
  display.display();
}


void loop()
{

  if (!clientMQTT.connected()) {
    reconnect();
  }
  clientMQTT.loop();

  if(shouldUpdateUI) {
    shouldUpdateUI = false;
    Serial.println("Printing on the display: " + value + " and " + percentage);
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(Roboto_Condensed_10);
    display.drawString(0, 0, "Current Balance");
    display.setFont(Roboto_Condensed_16);
    display.drawString(0, 10, percentage);
    display.setFont(Roboto_Condensed_24);
    display.drawString(0, 26, value);
    display.display();
  }
}

