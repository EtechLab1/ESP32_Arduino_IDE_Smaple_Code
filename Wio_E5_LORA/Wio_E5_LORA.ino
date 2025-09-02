/*
 * ================================================================
 * Project   : ESP32 ↔ Wio-E5 LoRa (UART Interrupt Mode)
 * Board     : ESP32
 * Module    : Seeed Studio Wio-E5 (LoRaWAN module)
 * ================================================================
 *
 * Features:
 *   - Communicates with Wio-E5 using UART1 (interrupt-based receive)
 *   - Sends AT commands to configure LoRa in TEST mode
 *   - Receives LoRa packets, parses hex payload, and stores per-node data
 *   - Supports multiple nodes (robots) with dynamic slot allocation
 *   - Example parser extracts NodeID + up to 6 parameters
 *
 * Author: Kamlesh Vasoya
 * Date  : 02/09/2025
 * ================================================================
 */

#include <HardwareSerial.h>

// -----------------------------
// System Configuration
// -----------------------------
#define MAX_NODE   3               // Maximum robots supported
const uint8_t Node_IDs[MAX_NODE] = {0x10, 0x11, 0x12};

// -----------------------------
// Control Variables
// -----------------------------
int currentNodeIndex = 0;                  // Index of current node being processed
unsigned long commandSentTime = 0;         // Time when command was sent
const unsigned long RESPONSE_TIMEOUT = 10000; // 10 seconds timeout for response
bool waitingForResponse = false;           // Flag: true if waiting for response

// -----------------------------
// Node Data Structure
// -----------------------------
struct ReceiveData {
  uint8_t responseId = 0;   // Response type ID
  uint8_t nodeId = 0;       // Robot Node ID
  uint8_t param_1 = 0;
  uint8_t param_2 = 0;
  uint8_t param_3 = 0;
  uint8_t param_4 = 0;
  uint8_t param_5 = 0;
  uint8_t param_6 = 0;
};

ReceiveData NodeData[MAX_NODE]; // Array to store data from all nodes

// =============================================================
//                   UART Configuration
// =============================================================

// Create UART1 instance for Wio-E5
HardwareSerial WioE5(1);

// Pin mapping for UART1
#define WIOE5_TX   27   // ESP32 TX -> Wio-E5 RX
#define WIOE5_RX   26   // ESP32 RX -> Wio-E5 TX
#define BAUDRATE   9600 // UART baud rate

// UART receive buffer
String command_data = "";
volatile uint8_t command_receiving = 0;    // Receiving in progress
volatile uint8_t command_ready = 0;        // Command ready for parsing

// =============================================================
//                   UART Receive Interrupt
// =============================================================
void onUartReceive() {
  char c = (char)WioE5.read();

  // Start of new packet marker
  if (c == '+') {
    command_data = "";       // Reset buffer
    command_receiving = 1;   // Start receiving
  }

  // If receiving → accumulate characters
  if (command_receiving) {
    if (c == '\n') {
      // End of line detected
      command_receiving = 0;
      command_ready = 1;
      Serial.println("Wio-E5: " + command_data);
    } else {
      command_data += c;
    }
  }

  // Validate response
  if (command_ready) {
    if (command_data.indexOf("\"59") == 10) {
      // Valid payload → keep flag set
      waitingForResponse = false;
      command_ready = 1;
    } else {
      // Invalid response → discard
      command_ready = 0;
    }
  }
}

// =============================================================
//                   Helper Function Prototypes
// =============================================================
void sendCommand(String cmd);
void InitLora(void);
void LoraRXMode(void);
ReceiveData* findOrCreateNode(uint8_t id);
bool parseAndStoreNodeResponse(String rawData);
void TestParse(void);
void sendNodeStatusCommand(uint8_t NodeId);

// =============================================================
//                        Setup Function
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize UART1 with RX interrupt enabled
  WioE5.begin(BAUDRATE, SERIAL_8N1, WIOE5_RX, WIOE5_TX);

  // Attach RX callback
  WioE5.onReceive(onUartReceive);

  Serial.println("UART interrupt RX enabled. Waiting for Wio-E5...");

  InitLora(); // Configure Wio-E5 for LoRa test mode
}

// =============================================================
//                        Main Loop
// =============================================================
void loop() {
  delay(1000);
}

// =============================================================
//                Wio-E5 AT Command Helpers
// =============================================================

// Send AT command to Wio-E5
void sendCommand(String cmd) {
  Serial.println("Sending: " + cmd);
  WioE5.print(cmd);
  WioE5.print("\r\n");
  delay(500);
}

// Initialize Wio-E5 in LoRa test mode
void InitLora(void) {
  sendCommand("AT");
  delay(10);
  sendCommand("AT+MODE=TEST");
  delay(10);
  sendCommand("AT+TEST=RFCFG,866,SF12,125,10,10,22,ON,OFF,OFF");
  delay(10);
}

// Put Wio-E5 into RX mode
void LoraRXMode(void) {
  sendCommand("AT+TEST=RXLRPKT");
  delay(10);
}

// =============================================================
//             Node Data Management Functions
// =============================================================

// Find or create slot for a given NodeID
ReceiveData* findOrCreateNode(uint8_t id) {
  // Search existing slots
  for (int i = 0; i < MAX_NODE; i++) {
    if (NodeData[i].nodeId == id) {
      return &NodeData[i];
    }
  }
  // Allocate new slot
  for (int i = 0; i < MAX_NODE; i++) {
    if (NodeData[i].nodeId == 0) {
      NodeData[i].nodeId = id;
      return &NodeData[i];
    }
  }
  return nullptr; // No space available
}

// Parse robot response and store data
bool parseAndStoreNodeResponse(String rawData) {
  // Extract hex string inside quotes
  int start = rawData.indexOf('"') + 1;
  int end   = rawData.lastIndexOf('"');
  if (start <= 0 || end <= start) return false;

  String data = rawData.substring(start, end); // e.g., "591011BA141F172F"
  if (data.length() < 16) return false;

  // Parse fields
  ReceiveData temp;
  temp.responseId = strtol(data.substring(0, 2).c_str(), NULL, 16);
  temp.nodeId     = strtol(data.substring(2, 4).c_str(), NULL, 16);
  temp.param_1    = strtol(data.substring(4, 6).c_str(), NULL, 16);
  temp.param_2    = strtol(data.substring(6, 8).c_str(), NULL, 16);
  temp.param_3    = strtol(data.substring(8, 10).c_str(), NULL, 16);
  temp.param_4    = strtol(data.substring(10, 12).c_str(), NULL, 16);
  temp.param_5    = strtol(data.substring(12, 14).c_str(), NULL, 16);
  temp.param_6    = strtol(data.substring(14, 16).c_str(), NULL, 16);

  // Store into NodeData
  ReceiveData* slot = findOrCreateNode(temp.nodeId);
  if (!slot) return false;
  *slot = temp;

  return true;
}

// =============================================================
//                 Testing and Debugging Helpers
// =============================================================

// Example parser test with sample responses
void TestParse(void) {
  String rx1 = "+TEST: RX \"591011BA141F172F\"";
  String rx2 = "+TEST: RX \"592022BB152F1830\"";
  String rx3 = "+TEST: RX \"593022BB152F1830\"";

  parseAndStoreNodeResponse(rx1);
  parseAndStoreNodeResponse(rx2);
  parseAndStoreNodeResponse(rx3);

  // Print stored node data
  for (int i = 0; i < MAX_NODE; i++) {
    if (NodeData[i].nodeId != 0) {
      Serial.print("Node ID: "); Serial.println(NodeData[i].nodeId, HEX);
      Serial.print("  param_1: "); Serial.println(NodeData[i].param_1);
      Serial.print("  param_2: "); Serial.println(NodeData[i].param_2);
      Serial.print("  param_3: "); Serial.println(NodeData[i].param_3);
    }
  }
}

// Send status command to a specific node
void sendNodeStatusCommand(uint8_t NodeId) {
  char payload[32];
  sprintf(payload, "AT+TEST=TXLRPKT,5A%x%x%x", NodeId, 0x11, 0x12);

  sendCommand(String(payload));
  Serial.print("Sent status to Robot ");
  Serial.println(NodeId, HEX);

  delay(700);
  LoraRXMode(); // Switch back to RX mode
  delay(500);
}
