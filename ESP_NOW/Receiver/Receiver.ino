/*
   Project   : ESP32 WiFi + ESP-NOW Receiver Example
   Board     : ESP32 Wrover
   IDE       : Arduino IDE 1.8.19
   ESP32 Core: v3.3.0

   Description:
     - Initializes ESP32 as Wi-Fi Station
     - Initializes ESP-NOW protocol
     - Receives data using ESP-NOW
     - Parses incoming structure and prints to Serial
*/

#include "ESP32_NOW.h"
#include "WiFi.h"

// === Data Structure to Match Sender ===
typedef struct test_struct {
  int x;
} test_struct;

// Create an instance of the struct to hold received data
test_struct test;

// === Callback: Runs when ESP-NOW data is received ===
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&test, incomingData, sizeof(test));  // Copy received data into 'test'

  Serial.print("Bytes received: ");
  Serial.println(len);

  Serial.print("Data: ");
  Serial.println(test.x);
}

void setup() {
  // === Initialize Serial Monitor ===
  Serial.begin(115200);

  // === Set Wi-Fi Mode to Station (required for ESP-NOW) ===
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Optional: Ensure not connected to any AP

  // === Initialize ESP-NOW ===
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // === Register the Receive Callback Function ===
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  // Nothing to do in loop — data is handled in callback
}
