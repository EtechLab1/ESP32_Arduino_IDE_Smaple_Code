/*
 * ================================================================
 * Project   : ESP32 Preferences (NVS) Example
 * Library   : Preferences.h
 * ================================================================
 *
 * Description:
 *   - Demonstrates saving, fetching, and updating persistent data
 *     using ESP32 NVS (Non-Volatile Storage).
 *   - Data persists across resets and power cycles.
 *
 * Usage:
 *   - Data_Fetch()   → Reads data if exists, else writes defaults.
 *   - defaultData()  → Initializes default values in NVS.
 *   - updateData()   → Updates stored values with new data.
 *
 * Author : [Your Name]
 * Date   : [Date]
 * ================================================================
 */

#include <Preferences.h>

// =============================================================
// Configuration
// =============================================================
#define RW_MODE false   // Read/Write mode
#define RO_MODE true    // Read-Only mode

Preferences preferences;   // Preferences instance

// =============================================================
// Application Data
// =============================================================
// These variables mirror what is stored in NVS
String   Str_data;
uint8_t  Int_data;
float    Float_data;

bool dataExist = false;    // Tracks whether valid data exists in NVS

// =============================================================
// Function Prototypes
// =============================================================
void Data_Fetch(void);
void defaultData(void);
void updateData(void);

// =============================================================
// Setup
// =============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 Preferences Example");
  delay(500);

  // Fetch existing data or create default values on first run
  Data_Fetch();

  // Print fetched data
  Serial.println("=== Current Stored Data ===");
  Serial.println("String : " + Str_data);
  Serial.println("Int    : " + String(Int_data));
  Serial.println("Float  : " + String(Float_data, 2));

  // Uncomment on first run if you want to overwrite stored values
  // updateData();
}

// =============================================================
// Main Loop
// =============================================================
void loop() {
  delay(3000);

  // Always fetch the latest saved data
  Data_Fetch();

  // Print stored data
  Serial.println("=== Stored Data (after fetch) ===");
  Serial.println("String : " + Str_data);
  Serial.println("Int    : " + String(Int_data));
  Serial.println("Float  : " + String(Float_data, 2));
}

// =============================================================
// Preferences Functions
// =============================================================

/**
 * @brief Fetch data from NVS.
 *        If not initialized, load default values.
 */
void Data_Fetch(void) {
  // Open Preferences in Read-Only mode
  preferences.begin("NV_Data", RO_MODE);

  // Check if data has been initialized before
  dataExist = preferences.isKey("dataExist");

  if (!dataExist) {
    Serial.println("⚠️  Data does not exist. Writing defaults...");
    preferences.end();
    defaultData();  // First run → store defaults
  } else {
    // Read stored values with default fallbacks
    Str_data   = preferences.getString("Str_data", "ABCDEFGH");
    Int_data   = preferences.getUInt("Int_data", 100);
    Float_data = preferences.getFloat("Float_data", 15.15);

    preferences.end();
  }
}

/**
 * @brief Store default values into NVS.
 */
void defaultData(void) {
  // Open Preferences in Read/Write mode
  preferences.begin("NV_Data", RW_MODE);

  preferences.putString("Str_data", "ABCDEFGH");
  preferences.putUInt("Int_data", 100);
  preferences.putFloat("Float_data", 15.15);
  preferences.putBool("dataExist", true);   // Marker for first-run check

  preferences.end();
  Serial.println("✅ Default values stored in NVS.");
}

/**
 * @brief Update stored data with new values.
 */
void updateData(void) {
  // Open Preferences in Read/Write mode
  preferences.begin("NV_Data", RW_MODE);

  preferences.putString("Str_data", "OPQRSTUVWXYZ");
  preferences.putUInt("Int_data", 250);
  preferences.putFloat("Float_data", 1234.56);

  preferences.end();
  Serial.println("✅ Stored values updated in NVS.");
}
