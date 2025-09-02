/*
   Project   : ESP32 WiFi + AWS IoT Core (MQTT) Example
   Board     : ESP32 Wrover
   IDE       : Arduino IDE 1.8.19
   ESP32 Core: v3.3.0

   Description:
     - Connects ESP32 to WiFi
     - Maintains AWS IoT MQTT secure connection
     - Publishes random test values to AWS IoT topic
     - Subscribes to a topic and handles incoming messages
     - LED ON when WiFi connected, OFF when disconnected
*/

#include "Secret.h"             // Contains AWS endpoint & certificates
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// -------------------- Pin Definitions --------------------
#define WIFI_LED_PIN 2          // On-board LED (GPIO2)

// -------------------- WiFi Credentials --------------------
const char* WIFI_SSID = "EngineiusTechnologies";   // WiFi SSID
const char* WIFI_PASS = "Etech@2022";           // WiFi Password

// -------------------- Global Variables --------------------
uint8_t R1;                       // Random value #1
uint8_t R2;                       // Random value #2

// Build the data string dynamically
char dataBuffer[50];

WiFiClientSecure net;           // Secure WiFi client for TLS
PubSubClient client(net);       // MQTT client (PubSubClient)

// -------------------- Connection Retry Timers --------------------
unsigned long lastAWSAttempt = 0;
unsigned long lastAWSPublish = 0;
const unsigned long AWS_RETRY_INTERVAL = 5000; // retry AWS every 5s
const unsigned long AWS_PUBLISH_INTERVAL = 2000; // publish AWS every 2s

// -------------------- Forward Declarations --------------------
void connectAWS();
void publishMessage();
void messageHandler(char* topic, byte* payload, unsigned int length);

// ============================================================
//                  WiFi Event Handlers
// ============================================================

// Called when the ESP32 successfully connects to WiFi
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, HIGH); // Turn LED ON
  Serial.println("📶 WiFi Connected Successfully!");
}

// Called when the ESP32 gets disconnected from WiFi
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, LOW);  // Turn LED OFF
  Serial.println("⚠️ WiFi Disconnected!");
  Serial.println("🔄 Attempting to reconnect...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// ============================================================
//                  AWS IoT Functions
// ============================================================

// Non-blocking AWS IoT Core connection
void connectAWS() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected, skipping AWS connect");
    return;
  }

  if (client.connected()) return; // Already connected

  // Retry only after interval
  if (millis() - lastAWSAttempt >= AWS_RETRY_INTERVAL) {
    lastAWSAttempt = millis(); // update timer

    // Configure TLS credentials once before connection
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.setCallback(messageHandler);

    Serial.print("🔗 Connecting to AWS IoT ... ");

    if (client.connect(THINGNAME)) {
      Serial.println("✅ Connected!");
      client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    } 
    else {
      Serial.print("⚠️ Failed, state=");
      Serial.println(client.state());
    }
  }
}

// Publish JSON message to AWS IoT topic
void publishMessage(uint8_t Robot_ID, uint8_t Data1, uint8_t Data2, uint8_t Data3) {
  StaticJsonDocument<200> doc;
  doc["device_id"] = "68a98dd44be2cd33ae33f8ad";
  doc["robot_id"] = Robot_ID;

  sprintf(dataBuffer, "START/%d/%d/%d", Data1, Data2, Data3);
  doc["data"] =  dataBuffer;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("📤 Message published to AWS IoT");
}

// Handle incoming messages from subscribed AWS IoT topic
void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("📥 Incoming message on topic: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  for(int i =0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (!err) {
    if (doc.containsKey("device_id")) {
      const char* device_id = doc["device_id"];
      uint8_t robot_id = doc["robot_id"];
      const char* data = doc["data"];
      Serial.print("➡️ Message content: ");
      Serial.print(device_id);
      Serial.print(", ");
      Serial.print(robot_id);
      Serial.print(", ");
      Serial.println(data);
    } 
    else {
      Serial.println("⚠️ No 'message' field in JSON!");
    }
  } else {
    Serial.print("❌ JSON Parse Error: ");
    Serial.println(err.f_str());
  }
}

// ============================================================
//                  Setup Function
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);

  WiFi.disconnect(true);
  delay(100);

  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// ============================================================
//                  Loop Function
// ============================================================
void loop() {
  // Keep MQTT alive
  if (client.connected()) {
    client.loop();
    if (millis() - lastAWSPublish >= AWS_PUBLISH_INTERVAL) {
      lastAWSPublish = millis(); // update timer
  
      // Generate and publish random test values
      R1 = random(1, 100);
      R2 = random(1, 100);
      Serial.printf("📊 Random_1: %d, Random_2: %d\n", R1, R2);
  
      publishMessage(11, R1, R2, R2);
    }
  }
  else {
    // Maintain AWS IoT connection (non-blocking)
    connectAWS();
  }
}
