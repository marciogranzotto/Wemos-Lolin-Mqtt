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

#define OLED_HEIGHT 64

SSD1306  display(0x3c, 5, 4);
WiFiMulti WiFiMulti;
WiFiClient wclient;
PubSubClient clientMQTT(wclient);

boolean shouldUpdateUI = false;

struct DisplayData {
  const char* line1;
  const char* line2;
  const char* line3;
  int line1Size;
  int line2Size;
  int line3Size;
};

DisplayData displayData = {"--", "--", "--", 10, 16, 24};

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

  const char* line1 = root["line1"];
  const char* line2 = root["line2"];
  const char* line3 = root["line3"];
  int line1Size = root["line1Size"];
  int line2Size = root["line2Size"];
  int line3Size = root["line3Size"];

  displayData = {line1, line2, line3, line1Size, line2Size, line3Size};
  
  shouldUpdateUI = true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clientMQTT.connect(CLIENT_ID)) {
      Serial.println("connected");
      // subscribe to the topic
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

    int freeSpace = OLED_HEIGHT - (displayData.line1Size + displayData.line2Size + displayData.line3Size);
    if (freeSpace < 0) {
      //this means that the last line will be out of the display
      freeSpace = 0;
    }
//    Serial.print("freeSpace size: ");
//    Serial.println(freeSpace);
    int line2Y = displayData.line1Size + freeSpace/2;
    int line3Y = displayData.line2Size + line2Y + freeSpace/2;
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    display.setFont(getFontForSize(displayData.line1Size));
    display.drawString(0, 0, displayData.line1);
    
    display.setFont(getFontForSize(displayData.line2Size));
    display.drawString(0, line2Y, displayData.line2);
    
    display.setFont(getFontForSize(displayData.line3Size));
    display.drawString(0, line3Y, displayData.line3);
    
    display.display();
  }
}

const unsigned char* getFontForSize(int fontSize) {
  switch (fontSize) {
    case 10:
      return Roboto_Condensed_10;
    case 24:
      return Roboto_Condensed_24;
    default:
      return Roboto_Condensed_16;
  }
}

