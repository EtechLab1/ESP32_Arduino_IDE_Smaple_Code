/*
 * ================================================================
 * Project   : ESP32 WiFi Auto-Reconnect + NTP Time Fetch
 * Board     : ESP32 Wrover
 * IDE       : Arduino IDE 1.8.19
 * ESP32 Core: v3.3.0
 * ================================================================
 *
 * Description:
 *   - Connects ESP32 to WiFi using stored credentials
 *   - Automatically reconnects if WiFi connection is lost
 *   - Controls an onboard LED: ON = Connected, OFF = Disconnected
 *   - Fetches NTP time and keeps it updated with reduced traffic
 *   - Displays current date/time over Serial Monitor
 *
 * Dependencies:
 *   - WiFi.h (ESP32 WiFi library)
 *   - time.h  (NTP + localtime functions)
 *
 * Author: [Your Name]
 * Date  : [Date]
 * ================================================================
 */

#include <WiFi.h>
#include <time.h>

// -------------------- Pin Definitions --------------------
#define WIFI_LED_PIN 2     // On-board LED (GPIO2)

// -------------------- WiFi Credentials --------------------
const char* WIFI_SSID = "EngineiusTechnologies";  // WiFi SSID
const char* WIFI_PASS = "Etech@2022";             // WiFi Password

// -------------------- NTP Server & Timezone --------------------
const char* NTP_SERVER = "ch.pool.ntp.org";       // NTP server
const char* TZ_INFO    = "IST-5:30";              // Timezone (India Standard Time)

// -------------------- Global Time Variables --------------------
tm timeinfo;                // Structure to hold time data
time_t now;                 // Current time in seconds since epoch
unsigned long lastNTPtime;  // Last synced NTP time
unsigned long lastEntryTime;// Last time millis() was updated

// Time components
uint8_t yy = 0, mm = 0, dd = 0;
uint8_t Hour = 0, Minute = 0, Second = 0;
uint8_t Day  = 0;

// =============================================================
//                   WiFi Event Handlers
// =============================================================

// Called when WiFi connects successfully
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, HIGH); // Turn LED ON
  Serial.println("📶 WiFi Connected Successfully!");
}

// Called when WiFi disconnects
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, LOW);  // Turn LED OFF
  Serial.println("⚠️ WiFi Disconnected!");
  Serial.println("🔄 Attempting to reconnect...");

  // Reconnect using stored credentials
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// =============================================================
//                        Setup Function
// =============================================================
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Configure WiFi LED pin
  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);

  // Reset previous WiFi state
  WiFi.disconnect(true);
  delay(1000);

  // Register WiFi event callbacks
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Start WiFi connection
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Configure NTP time
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);  // Apply timezone

  // Sync NTP time (wait up to 10 seconds)
  if (!getNTPtime(10)) {
    Serial.println("❌ NTP Time Sync Failed! Restarting ESP...");
    ESP.restart();
  }

  // Store initial time
  lastNTPtime   = time(&now);
  lastEntryTime = millis();
}

// =============================================================
//                        Loop Function
// =============================================================
void loop() {
  // Update time without excessive NTP requests
  getTimeReducedTraffic(600);

  // Show current time
  showTime(timeinfo);

  delay(200); // Refresh interval
}

// =============================================================
//                   TIME FETCH FUNCTIONS
// =============================================================

/**
 * @brief Update time without hitting NTP too frequently.
 * 
 * @param sec Interval (in seconds) before re-sync with NTP.
 */
void getTimeReducedTraffic(int sec) {
  tm *ptm;

  if ((millis() - lastEntryTime) < (1000 * sec)) {
    // Update time locally
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  } 
  else {
    // Resync with NTP
    lastEntryTime = millis();
    lastNTPtime   = time(&now);
    now           = lastNTPtime;
    Serial.println("🌐 Resynced with NTP server");
  }

  // Convert to local time
  ptm      = localtime(&now);
  timeinfo = *ptm;
}

/**
 * @brief Sync time from NTP server.
 * 
 * @param sec Maximum wait time in seconds.
 * @return true if successful, false otherwise.
 */
bool getNTPtime(int sec) {
  uint32_t start = millis();

  do {
    time(&now);
    localtime_r(&now, &timeinfo);
    Serial.print(".");
    delay(10);
  } while (((millis() - start) <= (1000 * sec)) && 
           (timeinfo.tm_year < (2016 - 1900)));

  if (timeinfo.tm_year <= (2016 - 1900)) {
    return false;  // NTP call failed
  }

  Serial.println("\n✅ Time Synced!");
  char time_output[30];
  strftime(time_output, sizeof(time_output), "%a %d-%m-%Y %T", localtime(&now));
  Serial.println(time_output);

  return true;
}

/**
 * @brief Print formatted date and time.
 * 
 * @param localTime Time structure (tm).
 */
void showTime(tm localTime) {
  // Extract date & time components
  dd     = localTime.tm_mday;
  mm     = localTime.tm_mon + 1;
  yy     = localTime.tm_year - 100;
  Hour   = localTime.tm_hour;
  Minute = localTime.tm_min;
  Second = localTime.tm_sec;
  Day    = (localTime.tm_wday == 0) ? 7 : localTime.tm_wday;

  // Print formatted time
  Serial.printf("%02d/%02d/%02d - %02d:%02d:%02d (Day %d)\n",
                dd, mm, yy, Hour, Minute, Second, Day);
}
