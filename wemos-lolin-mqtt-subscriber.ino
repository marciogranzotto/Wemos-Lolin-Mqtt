#include <Wire.h>
#include <SSD1306.h>
#include "RobotoFonts.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>
#include <DebounceEvent.h>

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
#define SHOULD_FLIP_SCREEN false
#define HAS_BUTTON true
#define HAS_MOTION_SENSOR false
#define SECONDS_TO_TURN_OFF 60
#define INVERT_COLORS false
/////////////////////////////////////////

#define ALIGN_TOPIC TOPIC "/align_right"
#define OLED_HEIGHT 64
#define OLED_WIDTH 128
#define MQTT_PACKET_SIZE 1024
#define JSON_BUFFER_SIZE MQTT_PACKET_SIZE

#if defined(HAS_BUTTON) && HAS_BUTTON
#define BUTTON_PIN 0
void buttonTriggered(uint8_t pin, uint8_t event, uint8_t count, uint16_t length);
DebounceEvent buttonEvent = DebounceEvent(BUTTON_PIN, buttonTriggered, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
#endif

#if defined(HAS_MOTION_SENSOR) && HAS_MOTION_SENSOR
#define SENSOR_PIN 15
#endif

struct DisplayData {
  const char* line1;
  const char* line2;
  const char* line3;
  int line1Size;
  int line2Size;
  int line3Size;
};

SSD1306  display(0x3c, 5, 4);
WiFiClient wclient;
PubSubClient clientMQTT(wclient);

// Functions
void motionSensorTriggered();
void reconnect();

DisplayData displayData = {"--", "--", "--", 10, 16, 24};
volatile boolean shouldUpdateUI = false;
volatile boolean sensorIsOff = true;
volatile boolean align_right = false;
volatile int currentPage = 0;
volatile unsigned long motionSensorLastTriggeredInMicros;
DynamicJsonDocument jsonObject(JSON_BUFFER_SIZE);

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message
  Serial.println("\n\ncallback called!");
  Serial.print("length: ");
  Serial.println(length);
  String topicStr = String(topic);
  Serial.print("Message arrived [");
  Serial.print(topicStr);
  Serial.print("] ");
  Serial.println();

  char charPayload[length+1];
  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    charPayload[i] = (char)payload[i];
    Serial.print(charPayload[i]);
  }
  charPayload[length] = '\0';


  if (topicStr == ALIGN_TOPIC) {
    Serial.println("parsing alignment!");
    String payloadStr = String(charPayload);
    align_right = payloadStr == "true";
    Serial.print("Align Right: ");
    Serial.print(align_right);
    Serial.print(" vs ");
    Serial.println(payloadStr);
    shouldUpdateUI = true;
    return;
  }

  if (topicStr == TOPIC) {
    Serial.println("deserializing Json!");
    DeserializationError error = deserializeJson(jsonObject, (const char*) charPayload);
    if (error) {
      Serial.println("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    Serial.println("success!");
    shouldUpdateUI = true;
    return;
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  display.init();
  if (SHOULD_FLIP_SCREEN) {
    display.flipScreenVertically();
  }
  if (INVERT_COLORS) {
    display.invertDisplay();
  }
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Roboto_Condensed_16);

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println();
  Serial.println();
  display.clear();
  display.drawString(0, 0, "Waiting for WiFi...");
  display.display();
  Serial.print("Waiting for WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
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

  // set MQTT broker and connect
  clientMQTT.setServer(BROKER, BROKER_PORT);
  clientMQTT.setCallback(callback);
  clientMQTT.setBufferSize(MQTT_PACKET_SIZE);
  display.clear();
  display.drawString(0, 0, "Connecting to broker...");
  display.display();
  reconnect();
  display.drawString(0, 16, "Connected!");
  display.drawString(0, 32, "Waiting for messages...");
  display.display();
  Serial.println("Waiting for messages...");

  #if defined(HAS_MOTION_SENSOR) && HAS_MOTION_SENSOR
  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(SENSOR_PIN, motionSensorTriggered, CHANGE);

  motionSensorLastTriggeredInMicros = micros();
  #endif

}

void loop() {
  if (!clientMQTT.connected()) {
    reconnect();
  }
  clientMQTT.loop();

  #if defined(HAS_BUTTON) && HAS_BUTTON
  buttonEvent.loop();
  #endif

  #if defined(HAS_MOTION_SENSOR) && HAS_MOTION_SENSOR
  long sensorTriggerAgo = (long)(micros() - motionSensorLastTriggeredInMicros);
  if (sensorIsOff && sensorTriggerAgo > (SECONDS_TO_TURN_OFF * 1000000)) {
    display.displayOff();
    return;
  }
  #endif

  display.displayOn();

  if (shouldUpdateUI) {
    Serial.println("\n\nUpdating UI");
    serializeJson(jsonObject, Serial);
    shouldUpdateUI = false;
    parseJsonForCurrentPage();
    int freeSpace = OLED_HEIGHT - (displayData.line1Size + displayData.line2Size + displayData.line3Size);
    if (freeSpace < 0) {
      //this means that the last line will be out of the display
      freeSpace = 0;
    }
    int line2Y = displayData.line1Size + freeSpace / 2;
    int line3Y = displayData.line2Size + line2Y + freeSpace / 2;

    display.clear();
    int x = 0;
    if (align_right) {
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      x = OLED_WIDTH;
    } else {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      x = 0;
    }
    display.setFont(getFontForSize(displayData.line1Size));
    display.drawString(x, 0, displayData.line1);
    Serial.println(displayData.line1);

    display.setFont(getFontForSize(displayData.line2Size));
    display.drawString(x, line2Y, displayData.line2);
    Serial.println(displayData.line2);

    display.setFont(getFontForSize(displayData.line3Size));
    display.drawString(x, line3Y, displayData.line3);
    Serial.println(displayData.line3);

    display.display();
  }
}

void parseJsonForCurrentPage() {
  JsonObject page = jsonObject["pages"][currentPage];
  const char* line1 = page["1"]["text"];
  const char* line2 = page["2"]["text"];
  const char* line3 = page["3"]["text"];
  int line1Size = parseSize(page["1"]["size"]);
  int line2Size = parseSize(page["2"]["size"]);
  int line3Size = parseSize(page["3"]["size"]);

  displayData = {line1, line2, line3, line1Size, line2Size, line3Size};
}

int lastPageNumer() {
  JsonArray pages = jsonObject["pages"];
  return pages.size();
}

int parseSize(int intendedSize) {
  switch (intendedSize) {
    case 10:
    return 10;
    case 24:
    return 24;
    default:
    return 16;
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

#if defined(HAS_BUTTON) && HAS_BUTTON
void buttonTriggered(uint8_t pin, uint8_t event, uint8_t count, uint16_t length) {
  if (event == EVENT_PRESSED && HAS_BUTTON) {
    currentPage++;
    if (currentPage >= lastPageNumer()) {
      currentPage = 0;
    }
    // we should also reset the motion timer, since you are clicking the buttonEvent
    motionSensorLastTriggeredInMicros = micros();
    shouldUpdateUI = true;
  }
}
#endif


#if defined(HAS_MOTION_SENSOR) && HAS_MOTION_SENSOR
void motionSensorTriggered() {
  Serial.println("Sensor Triggered!");
  Serial.println("Sensor state: ");
  Serial.println(digitalRead(SENSOR_PIN));

  if (digitalRead(SENSOR_PIN) == HIGH) {
    sensorIsOff = false;
    shouldUpdateUI = true;
  } else {
    sensorIsOff = true;
    Serial.println("Updating Sensor timestamp");
    motionSensorLastTriggeredInMicros = micros();
    shouldUpdateUI = true;
  }
}
#endif

void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    clientMQTT.setCallback(callback);
    if (clientMQTT.connect(CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      // subscribe to the topic
      clientMQTT.subscribe(TOPIC, 1);
      Serial.print("subscribed to ");
      Serial.println(TOPIC);
      clientMQTT.subscribe(ALIGN_TOPIC, 1);
      Serial.print("subscribed to ");
      Serial.println(ALIGN_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
