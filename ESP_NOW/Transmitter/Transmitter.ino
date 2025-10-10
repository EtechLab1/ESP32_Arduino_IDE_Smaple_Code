/*
   Project   : ESP32 WiFi + ESP-NOW Transmitter Example
   Board     : ESP32 Wrover
   IDE       : Arduino IDE 1.8.19
   ESP32 Core: v3.3.0

   Description:
     - Initializes ESP32 in Wi-Fi Station mode
     - Initializes ESP-NOW protocol
     - Sends data to a paired ESP32 device upon button press
     - Monitors and reports transmission success via callback
*/

#include "ESP32_NOW.h"
#include "WiFi.h"

// === GPIO Pin Definitions ===
#define BTN_PIN 39  // Input pin for button

// === ESP-NOW Peer MAC Address (Receiver) ===
uint8_t broadcastAddress[] = {0x14, 0x33, 0x5C, 0x4C, 0x57, 0xB8};

// === Data Structure to Send ===
typedef struct struct_message {
  int A;  // Payload variable
} struct_message;

// === Create Instance of Data Structure ===
struct_message Data;

// === Button State Variables ===
bool BTN_CS = 1;  // Current state
bool BTN_LS = 1;  // Last state

// === Callback: Called After Sending Data ===
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  // === Initialize Serial Monitor ===
  Serial.begin(115200);

  // === Set Button Pin as Input ===
  pinMode(BTN_PIN, INPUT);

  // === Set Device as Wi-Fi Station (required for ESP-NOW) ===
  WiFi.mode(WIFI_STA);

  // === Initialize ESP-NOW ===
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // === Register Callback to Track Send Status ===
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  // === Configure Peer Info ===
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  // Set peer MAC
  peerInfo.channel = 0;                             // Default channel
  peerInfo.encrypt = false;                         // No encryption
  peerInfo.ifidx = WIFI_IF_STA;                     // Interface: Station mode

  // === Add Peer ===
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // === Read Current Button State ===
  BTN_CS = digitalRead(BTN_PIN);

  // === On Button Press Event (falling edge) ===
  if ((BTN_LS != BTN_CS) && (BTN_CS == 0)) {
    Data.A = 1;  // Set data value to send

    // === Send Data via ESP-NOW ===
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&Data, sizeof(Data));
  }

  // === Update Last Button State ===
  BTN_LS = BTN_CS;

  delay(50);  // Debounce delay
}
