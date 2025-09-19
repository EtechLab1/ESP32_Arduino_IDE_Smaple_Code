/*
 * ======================================================================
 * Project   : ESP32 ↔ Quectel EC200U-CN (UART Interrupt Mode)
 * Board     : ESP32
 * Module    : Quectel EC200U-CN (GSM + GPS)
 * Author    : Kamlesh Vasoya
 * Date      : 19/09/2025
 * ======================================================================
 *
 * Description:
 *   - Communicates with EC200U-CN using UART1 (interrupt-based receive).
 *   - Sends AT commands to configure GPRS and enable GNSS (GPS).
 *   - Periodically requests GPS location and parses the response.
 *
 * ======================================================================
 */

#include <HardwareSerial.h>

// ----------------------------------------------------------------------
// Pin Mapping & Configuration
// ----------------------------------------------------------------------
#define EC200_TX_PIN    26          // ESP32 TX  → EC200 RX
#define EC200_RX_PIN    27          // ESP32 RX  → EC200 TX
#define EC200_BAUDRATE  115200      // UART baud rate

// Timing variables
unsigned long gprsInterval   = 4000; // Interval between GPS requests (ms)
unsigned long previousMillis = 0;    // Stores last GPS request timestamp

// UART interface
HardwareSerial EC200Serial(1);       // Use UART1 for EC200 communication

// UART receive buffer
String uartBuffer = "";

// ----------------------------------------------------------------------
// Data Structure: Holds parsed GPS information
// ----------------------------------------------------------------------
struct GPSData {
  String utcTime;      // UTC time (HHMMSS.sss)
  float latitude;      // Latitude (decimal degrees)
  float longitude;     // Longitude (decimal degrees)
  float hdop;          // Horizontal dilution of precision
  float altitude;      // Altitude (meters)
  int   fixMode;       // Fix mode: 2 = 2D, 3 = 3D
  float course;        // Course over ground (degrees)
  float speed;         // Speed (km/h)
  float climbRate;     // Climb rate (m/s)
  String date;         // Date (DDMMYY)
  int   viewSatellites;// Number of satellites in view
};

// ----------------------------------------------------------------------
// Function Prototypes
// ----------------------------------------------------------------------
void initializeEC200();
void sendATCommand(const String &command);
void configureGPRS();
void enableGNSS();
void requestGPSLocation();
void readUARTData();
GPSData parseQGPSLOC(const String &qgpsloc);
void exampleUsage(const String &qgpsloc);

// ======================================================================
// SETUP: Initialize Serial ports and EC200 module
// ======================================================================
void setup() {
  Serial.begin(115200);  // Debug monitor
  EC200Serial.begin(EC200_BAUDRATE, SERIAL_8N1, EC200_RX_PIN, EC200_TX_PIN);

  delay(1000);           // Allow peripherals to stabilize
  initializeEC200();     // Start EC200 configuration
}

// ======================================================================
// LOOP: Periodically request GPS data and handle UART input
// ======================================================================
void loop() {
  // Request GPS location at defined intervals
  if (millis() - previousMillis > gprsInterval) {
    previousMillis = millis();
    requestGPSLocation();
  }

  // Continuously read incoming UART data
  readUARTData();
}

// ======================================================================
// FUNCTION DEFINITIONS
// ======================================================================

/**
 * @brief Initializes the EC200U-CN module and enables GNSS.
 */
void initializeEC200() {
  Serial.println("Initializing EC200U-CN module...");
  sendATCommand("AT");       // Basic AT check
  delay(1000);

  configureGPRS();           // Set APN and activate PDP context
  enableGNSS();              // Turn on GNSS module
}

/**
 * @brief Sends an AT command to EC200.
 * @param command The AT command string (no CRLF required).
 */
void sendATCommand(const String &command) {
  Serial.println("Sending: " + command);
  EC200Serial.print(command);
  EC200Serial.print("\r\n"); // Append carriage return + newline
  delay(100);                // Allow time for processing
}

/**
 * @brief Configures APN and activates PDP context for GPRS.
 */
void configureGPRS() {
  Serial.println("Configuring APN...");
  sendATCommand("AT+QICSGP=1,1,\"www\"");
  delay(1000);

  Serial.println("Activating PDP context...");
  sendATCommand("AT+QIACT=1");
  delay(5000);

  sendATCommand("AT+QIACT?"); // Verify PDP context
  delay(1000);
}

/**
 * @brief Enables GNSS functionality on EC200U-CN.
 */
void enableGNSS() {
  Serial.println("Turning ON GNSS...");
  sendATCommand("AT+QGPS=1");
  delay(1000);
}

/**
 * @brief Requests current GPS location from EC200U-CN.
 */
void requestGPSLocation() {
  Serial.println("Requesting GPS location...");
  sendATCommand("AT+QGPSLOC=2");
  delay(1000);
}

/**
 * @brief Reads incoming UART data and checks for GPS data.
 */
void readUARTData() {
  while (EC200Serial.available()) {
    uartBuffer = EC200Serial.readStringUntil('\n');
    uartBuffer.trim(); // Remove extra spaces or CRLF

    if (!uartBuffer.isEmpty()) {
      Serial.println("Received: " + uartBuffer);

      if (uartBuffer.indexOf("+QGPSLOC:") >= 0) {
        // Parse and display GPS information
        exampleUsage(uartBuffer);
      } else {
        Serial.println("No GPS Data Available...");
      }
    }
  }
}

/**
 * @brief Parses a +QGPSLOC response string into a GPSData struct.
 * @param qgpsloc Example: "+QGPSLOC: 061122.000,21.21429,72.88421,1.3,-1.3,3,000.00,0.6,0.3,170925,06"
 * @return GPSData Parsed GPS data.
 */
GPSData parseQGPSLOC(const String &qgpsloc) {
  GPSData data;

  // Remove prefix "+QGPSLOC: " if present
  String payload = qgpsloc;
  payload.replace("+QGPSLOC: ", "");
  payload.trim();

  // Split fields by comma
  int index = 0, lastPos = 0, field = 0;

  while ((index = payload.indexOf(',', lastPos)) != -1) {
    String token = payload.substring(lastPos, index);
    token.trim();

    switch (field) {
      case 0: data.utcTime   = token;          break;
      case 1: data.latitude  = token.toFloat();break;
      case 2: data.longitude = token.toFloat();break;
      case 3: data.hdop      = token.toFloat();break;
      case 4: data.altitude  = token.toFloat();break;
      case 5: data.fixMode   = token.toInt();  break;
      case 6: data.course    = token.toFloat();break;
      case 7: data.speed     = token.toFloat();break;
      case 8: data.climbRate = token.toFloat();break;
    }
    lastPos = index + 1;
    field++;
  }

  // Handle remaining fields: date and satellites
  int nextComma = payload.indexOf(',', lastPos);
  if (nextComma != -1) {
    data.date           = payload.substring(lastPos, nextComma);
    data.viewSatellites = payload.substring(nextComma + 1).toInt();
  } else {
    // Edge case: last two fields combined
    String remaining = payload.substring(lastPos);
    int sep = remaining.indexOf(',');
    if (sep != -1) {
      data.date           = remaining.substring(0, sep);
      data.viewSatellites = remaining.substring(sep + 1).toInt();
    }
  }

  return data;
}

/**
 * @brief Example usage of parsed GPS data.
 * @param qgpsloc The raw +QGPSLOC response string.
 */
void exampleUsage(const String &qgpsloc) {
  GPSData gps = parseQGPSLOC(qgpsloc);

  Serial.println();
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~ NEW GPS DATA ~~~~~~~~~~~~~~~~~~~~~~~~");
  Serial.println("Parsed GPS Data:");
  Serial.println("UTC Time   : " + gps.utcTime);
  Serial.print  ("Latitude   : "); Serial.println(gps.latitude, 5);
  Serial.print  ("Longitude  : "); Serial.println(gps.longitude, 5);
  Serial.print  ("HDOP       : "); Serial.println(gps.hdop);
  Serial.print  ("Altitude   : "); Serial.println(gps.altitude);
  Serial.print  ("Fix Mode   : "); Serial.println(gps.fixMode);
  Serial.print  ("Course     : "); Serial.println(gps.course);
  Serial.print  ("Speed      : "); Serial.println(gps.speed);
  Serial.print  ("Climb Rate : "); Serial.println(gps.climbRate);
  Serial.println("Date       : " + gps.date);
  Serial.print  ("Satellites : "); Serial.println(gps.viewSatellites);
}
