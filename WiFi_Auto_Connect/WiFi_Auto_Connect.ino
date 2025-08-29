/*
 * Project   : ESP32 WiFi Auto-Reconnect Example
 * Board     : ESP32 Wrover
 * IDE       : Arduino IDE 1.8.19
 * ESP32 Core: v3.3.0
 *
 * Description:
 *   - Connects to a WiFi network with stored credentials
 *   - Turns ON an LED when connected
 *   - Turns OFF the LED when disconnected
 *   - Automatically attempts to reconnect if disconnected
 */

#include <WiFi.h>   // ESP32 WiFi library

// -------------------- Pin Definitions --------------------
#define WIFI_LED_PIN 2   // On-board LED (GPIO2)

// -------------------- WiFi Credentials --------------------
const char* WIFI_SSID = "Kamlesh's S20 FE";   // WiFi SSID
const char* WIFI_PASS = "ghdo1401";           // WiFi Password

// -------------------- WiFi Event Handlers --------------------

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

  // Try reconnecting with stored credentials
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// -------------------- Setup Function --------------------
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Configure WiFi LED pin
  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);

  // Reset any previous WiFi state
  WiFi.disconnect(true);
  delay(1000);

  // Register WiFi event callbacks (new style for ESP32 v3.x)
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Start WiFi connection
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// -------------------- Loop Function --------------------
void loop() {
  // Nothing needed here; WiFi events handle everything
  delay(1000);
}
