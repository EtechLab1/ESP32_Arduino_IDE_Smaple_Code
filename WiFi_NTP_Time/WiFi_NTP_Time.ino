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
 *   - Onboard LED: ON = WiFi connected, OFF = disconnected
 *   - Initializes NTP only once per WiFi connection
 *   - Keeps time updated with minimal NTP requests
 *   - Prints current date/time on Serial Monitor
 *
 * Dependencies:
 *   - WiFi.h (ESP32 WiFi library)
 *   - time.h  (NTP + localtime functions)
 *
 * Author : [Your Name]
 * Date   : [Date]
 * ================================================================
 */

#include <WiFi.h>
#include <time.h>

// -------------------- Pin Definitions --------------------
#define WIFI_LED_PIN 2   // On-board LED (GPIO2)

// -------------------- WiFi Credentials -------------------
const char* WIFI_SSID = "EngineiusTechnologies";
const char* WIFI_PASS = "Etech@2022";

// -------------------- NTP Server & Timezone ---------------
const char* NTP_SERVER = "pool.ntp.org"; // Public NTP pool
const char* TZ_INFO    = "IST-5:30";    // India Standard Time

// -------------------- Global Time Variables ----------------
tm timeinfo;                  // Structure to hold local time
time_t now;                   // Current time (epoch seconds)
unsigned long lastNTPtime;    // Last time synced from NTP
unsigned long lastEntryTime;  // Last millis() checkpoint
bool ntpInitialized = false;  // Tracks whether NTP was initialized

// Extracted time components
uint8_t yy = 0, mm = 0, dd = 0;
uint8_t Hour = 0, Minute = 0, Second = 0;
uint8_t Day  = 0;

// -------------------- Function Prototypes -----------------
void getTimeReducedTraffic(int sec);
bool getNTPtime(int sec);
void showTime(tm localTime);
void initializeNTP(void);

// =============================================================
//                   WiFi Event Handlers
// =============================================================

/**
 * @brief Called when WiFi connects successfully.
 */
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, HIGH); // Turn LED ON
  Serial.println("📶 WiFi Connected Successfully!");
}

/**
 * @brief Called when WiFi disconnects.
 */
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  digitalWrite(WIFI_LED_PIN, LOW);  // Turn LED OFF
  Serial.println("⚠️ WiFi Disconnected!");
  Serial.println("🔄 Attempting to reconnect...");

  // Attempt reconnection
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Force NTP re-initialization after reconnect
  ntpInitialized = false;
}

// =============================================================
//                        Setup Function
// =============================================================
void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Setup WiFi LED
  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);

  // Reset any previous WiFi state
  WiFi.disconnect(true);
  delay(1000);

  // Register event callbacks
  WiFi.onEvent(onWifiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // Start WiFi connection
  Serial.print("🔌 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  delay(100);
}

// =============================================================
//                        Loop Function
// =============================================================
void loop() {
  // Run only if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    // Perform NTP initialization once after WiFi connects
    if (!ntpInitialized) {
      ntpInitialized = true;
      delay(1500);       // Allow WiFi stack to stabilize
      initializeNTP();   // First sync with NTP
    } 
    else {
      // Update time with reduced NTP traffic
      getTimeReducedTraffic(600);

      // Print formatted time
      showTime(timeinfo);
    }
  }

  delay(200); // Main loop refresh rate
}

// =============================================================
//                   TIME MANAGEMENT FUNCTIONS
// =============================================================

/**
 * @brief Keeps time updated with minimal NTP requests.
 * 
 * @param sec Interval (in seconds) before re-syncing with NTP.
 */
void getTimeReducedTraffic(int sec) {
  if ((millis() - lastEntryTime) < (1000 * sec)) {
    // Estimate time using millis() since last NTP sync
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  } else {
    // Perform a resync with NTP
    lastEntryTime = millis();
    lastNTPtime   = time(&now);
    now           = lastNTPtime;
    Serial.println("🌐 Resynced with NTP server");
  }

  // Update time structure
  timeinfo = *localtime(&now);
}

/**
 * @brief Syncs time from the NTP server.
 * 
 * @param sec Maximum wait time in seconds.
 * @return true if sync was successful, false otherwise.
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
    return false;  // NTP sync failed
  }

  Serial.println("\n✅ Time Synced!");
  char time_output[30];
  strftime(time_output, sizeof(time_output), "%a %d-%m-%Y %T", localtime(&now));
  Serial.println(time_output);

  return true;
}

/**
 * @brief Displays formatted date and time over Serial Monitor.
 * 
 * @param localTime The local time structure (tm).
 */
void showTime(tm localTime) {
  // Extract components
  dd     = localTime.tm_mday;
  mm     = localTime.tm_mon + 1;
  yy     = localTime.tm_year - 100;
  Hour   = localTime.tm_hour;
  Minute = localTime.tm_min;
  Second = localTime.tm_sec;
  Day    = (localTime.tm_wday == 0) ? 7 : localTime.tm_wday;

  // Print formatted time string
  Serial.printf("%02d/%02d/%02d - %02d:%02d:%02d (Day %d)\n",
                dd, mm, yy, Hour, Minute, Second, Day);
}

/**
 * @brief Initializes NTP and performs the first sync attempt.
 */
void initializeNTP(void) {
  Serial.println("⏳ Initializing NTP Server...");

  // Configure NTP server and timezone
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);

  // Attempt NTP sync (first try)
  if (!getNTPtime(10)) {
    Serial.println("❌ NTP Time Sync Failed on first try... Retrying");
    delay(1000);

    // Second attempt before giving up
    if (!getNTPtime(10)) {
      Serial.println("❌ NTP Time Sync Failed! Restarting ESP...");
      ESP.restart();
    }
  }

  // Save reference times
  lastNTPtime   = time(&now);
  lastEntryTime = millis();

  Serial.println("✅ Initial NTP Sync Completed");
}
