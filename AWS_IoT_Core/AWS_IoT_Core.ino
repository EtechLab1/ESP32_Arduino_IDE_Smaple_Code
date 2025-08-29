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
const char* WIFI_SSID = "Kamlesh's S20 FE";   // WiFi SSID
const char* WIFI_PASS = "ghdo1401";           // WiFi Password

// -------------------- Global Variables --------------------
float R1;                       // Random value #1
float R2;                       // Random value #2

WiFiClientSecure net;           // Secure WiFi client for TLS
PubSubClient client(net);       // MQTT client (PubSubClient)

// -------------------- Connection Retry Timers --------------------
unsigned long lastAWSAttempt = 0;
const unsigned long AWS_RETRY_INTERVAL = 5000; // retry AWS every 5s

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
    } else {
      Serial.print("⚠️ Failed, state=");
      Serial.println(client.state());
    }
  }
}

// Publish JSON message to AWS IoT topic
void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["Random_1"] = R1;
  doc["Random_2"] = R2;

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

  if (!err) {
    if (doc.containsKey("message")) {
      const char* message = doc["message"];
      Serial.print("➡️ Message content: ");
      Serial.println(message);
    } else {
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

    // Generate and publish random test values
    R1 = random(1, 100);
    R2 = random(1, 100);
    Serial.printf("📊 Random_1: %.2f, Random_2: %.2f\n", R1, R2);

    publishMessage();
    delay(2000); // Publish frequency
  }
  else {
    // Maintain AWS IoT connection (non-blocking)
    connectAWS();
  }
}
