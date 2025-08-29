/*
 * Project   : ESP32 Multi-WiFi Auto-Reconnect Example
 * Board     : ESP32 Wrover
 * IDE       : Arduino IDE 1.8.19
 * ESP32 Core: v3.3.0
 *
 * Description:
 *   - Stores multiple WiFi credentials
 *   - Tries to connect in order until one is successful
 *   - Turns ON LED when connected
 *   - Turns OFF LED when disconnected
 *   - Reconnects automatically if WiFi drops
 */

#include <WiFi.h>

// -------------------- Pin Definitions --------------------
#define WIFI_LED_PIN 2   // On-board LED (GPIO2)

// -------------------- WiFi Credentials --------------------
// Add multiple SSID and Password pairs here
struct WiFiCred {
  const char* ssid;
  const char* pass;
};

WiFiCred wifiList[] = {
  {"Kamlesh's S20 FE", "ghdo1401"},
  {"EngineiusTechnologies", "Etech@2022"}
};

const int wifiCount = sizeof(wifiList) / sizeof(wifiList[0]);
int currentWiFi = 0;  // Track which WiFi network is being tried

// -------------------- WiFi Event Handlers --------------------

// Called when ESP32 connects successfully
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, HIGH);
  Serial.print("📶 Connected to: ");
  Serial.println(wifiList[currentWiFi].ssid);
}

// Called when ESP32 gets disconnected
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, LOW);
  Serial.println("⚠ WiFi Disconnected!");

  // Try next WiFi in the list
  currentWiFi = (currentWiFi + 1) % wifiCount;

  Serial.print("🔄 Attempting to connect to: ");
  Serial.println(wifiList[currentWiFi].ssid);

  WiFi.begin(wifiList[currentWiFi].ssid, wifiList[currentWiFi].pass);
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);

  // Reset WiFi state
  WiFi.disconnect(true);
  delay(500);

  // Register event handlers
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Start with the first WiFi
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(wifiList[currentWiFi].ssid);

  WiFi.begin(wifiList[currentWiFi].ssid, wifiList[currentWiFi].pass);
}

// -------------------- Loop --------------------
void loop() {
  // Nothing needed — WiFi events handle reconnect logic
  delay(1000);
}
