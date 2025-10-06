/*
 * ======================================================================
 * Project   : ESP32 ↔ Quectel EC200U-CN (MQTT HiveMQ)
 * Board     : ESP32
 * Module    : Quectel EC200U-CN (GSM + GPS + MQTT)
 * Author    : Kamlesh Vasoya
 * Date      : 19/09/2025
 * ======================================================================
 *
 * Description:
 *   - Communicates with EC200U-CN over UART1.
 *   - Configures GPRS for internet connectivity.
 *   - Establishes MQTT connection to HiveMQ broker.
 *   - Subscribes to a topic and periodically publishes messages.
 *
 * ======================================================================
 */

#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "secrate.h"

// ----------------------------------------------------------------------
// Pin Mapping & Configuration
// ----------------------------------------------------------------------
#define EC200_TX_PIN    26          // ESP32 TX → EC200 RX
#define EC200_RX_PIN    27          // ESP32 RX → EC200 TX
#define EC200_BAUDRATE  115200      // UART baud rate

// Timing variables
unsigned long publishInterval = 4000; // Interval between publishes (ms)
unsigned long lastPublishTime = 0;    // Timestamp of last publish

// UART interface
HardwareSerial EC200Serial(1);

// UART receive buffer
String uartBuffer = "";

// ----------------------------------------------------------------------
// Data structure to hold parsed MQTT message information
// ----------------------------------------------------------------------
struct MQTTMessage {
  int clientId;        // Client ID (e.g., 0)
  int messageId;       // Message ID (e.g., 0)
  String topic;        // Subscribed topic (e.g., "EC200_SUB")
  String payload;      // Message payload (e.g., "123456789")
};

// ----------------------------------------------------------------------
// Function Prototypes
// ----------------------------------------------------------------------
void setupEC200Module();      // Initialize EC200 and GPRS
void sendATCommand(const String &command);  // Send AT commands
void configureGPRS();         // Configure APN and activate PDP context
void connectToMQTTBroker();   // Connect to HiveMQ broker
void subscribeToTopic();      // Subscribe to MQTT topic
void publishMessage();        // Publish MQTT message
void handleUARTData();        // Handle incoming UART data

// ======================================================================
// SETUP: Initialize Serial ports and EC200 module
// ======================================================================
void setup() {
  Serial.begin(115200);
  EC200Serial.begin(EC200_BAUDRATE, SERIAL_8N1, EC200_RX_PIN, EC200_TX_PIN);

  delay(1000);
  setupEC200Module();     // Initialize GPRS
  connectToMQTTBroker();  // Connect to HiveMQ broker
  subscribeToTopic();     // Subscribe to topic
}

// ======================================================================
// LOOP: Periodically publish MQTT messages and process incoming data
// ======================================================================
void loop() {
  if (millis() - lastPublishTime > publishInterval) {
    lastPublishTime = millis();
    publishMessage();
  }
  handleUARTData();
}

// ======================================================================
// FUNCTION DEFINITIONS
// ======================================================================

/**
 * @brief Initializes EC200 module and configures GPRS.
 */
void setupEC200Module() {
  // Basic AT check to verify communication
  sendATCommand("AT");
  delay(1000);

  configureGPRS();
}

/**
 * @brief Sends an AT command.
 */
void sendATCommand(const String &command) {
  Serial.print("[SEND] ");
  Serial.println(command);
  EC200Serial.print(command);
  EC200Serial.print("\r\n");
  delay(100);
  handleUARTData();
}

/**
 * @brief Configures APN and activates PDP context for GPRS connectivity.
 */
void configureGPRS() {
  // Set APN profile: profile ID=1, PDP type=1 (IPv4), APN="www"
  sendATCommand("AT+QICSGP=1,1,\"www\"");
  delay(1000);

  // Activate PDP context to establish data connection
  sendATCommand("AT+QIACT=1");
  delay(5000);

  // Query PDP context status to verify activation
  sendATCommand("AT+QIACT?");
  delay(1000);
}

/**
 * @brief Opens an MQTT connection to HiveMQ and connects with client ID.
 */
void connectToMQTTBroker() {
  sendATCommand("AT+QFDEL=\"*\"");
  delay(5000);

  sendATCommand("AT+QMTCFG=\"SSL\",0,1,2");
  delay(5000);

  uint32_t certLength = strlen(AWS_CERT_CA);
  Serial.print("Length of AWS_CERT_CA: ");
  Serial.println(certLength);
  sendATCommand("AT+QFUPL=\"UFS:cacert.pem\",1188,100");
  sendATCommand(AWS_CERT_CA);
  delay(5000);
  
  sendATCommand("AT+QFUPL=\"UFS:client.pem\",1220,100");
  sendATCommand(AWS_CERT_CRT);
  delay(5000);
  sendATCommand("AT+QFUPL=\"UFS:user_key.pem\",1679,100");
  sendATCommand(AWS_CERT_PRIVATE);
  delay(5000);

  sendATCommand("AT+QSSLCFG=\"cacert\",2,\"UFS:cacert.pem\"");
  delay(5000);

  sendATCommand("AT+QSSLCFG=\"clientcert\",2,\"UFS:client.pem\"");
  delay(5000);

  sendATCommand("AT+QSSLCFG=\"clientkey\",2,\"UFS:user_key.pem\"");
  delay(5000);



  sendATCommand("AT+QSSLCFG=\"seclevel\",2,2");
  delay(1000);
  sendATCommand("AT+QSSLCFG=\"sslversion\",2,3");
  delay(1000);
  sendATCommand("AT+QSSLCFG=\"ciphersuite\",2,0xFFFF");
  delay(1000);
  sendATCommand("AT+QSSLCFG=\"ignorelocaltime\",1");
  delay(1000);


  //a2uf6iaxr3hfog-ats.iot.us-east-1.amazonaws.com
  sendATCommand("AT+QMTOPEN=0,\"a2uf6iaxr3hfog-ats.iot.us-east-1.amazonaws.com\",8883"); 
  delay(5000);
  
  sendATCommand("AT+QMTCONN=0,\"gsm_mqtt\""); 
  delay(5000);

//  sendCommand("AT+QMTDISC=0"); 
//  delay(1000);
//  sendCommand("AT+QMTCLOSE=0"); 
//  delay(1000);
}

/**
 * @brief Subscribes to a HiveMQ topic.
 */
void subscribeToTopic() {
  // Subscribe to topic: client 0, message ID=1, topic="EC200_SUB", QoS=1
  sendATCommand("AT+QMTSUB=0,1,\"EC200_SUB\",1");
  delay(1000);
}

/**
 * @brief Publishes a message to a topic.
 */
void publishMessage() {
  // Create JSON object
  StaticJsonDocument<200> doc;
  doc["Latitude"] = "21.2345";
  doc["Longitude"] = "72.4569";

  // Serialize JSON to buffer
  char jsonBuffer[256];
  size_t jsonLength = serializeJson(doc, jsonBuffer);

  // Use the actual length of the JSON string as the payload length
  String atCommand = "AT+QMTPUBEX=0,1,1,0,\"BG950_PUB\"," + String(jsonLength);

  // Send AT command to publish
  sendATCommand(atCommand.c_str());
  delay(400);

  // Send the JSON payload
  sendATCommand(jsonBuffer);
  delay(400);
}

/**
 * @brief Handles incoming UART data (e.g., MQTT messages or status).
 */
void handleUARTData() {
  while (EC200Serial.available()) {
    uartBuffer = EC200Serial.readStringUntil('\n');
    uartBuffer.trim();

    if (!uartBuffer.isEmpty()) {
      Serial.println("[RECV] " + uartBuffer);

      // Check for MQTT message reception
      if (uartBuffer.indexOf("+QMTRECV:") >= 0) {
        Serial.println("[INFO] MQTT message received from broker");
        // Additional parsing or handling can be done here
        handleMQTTMessage(uartBuffer);
      }
    }
  }
}

/**
 * @brief Parse a +QMTRECV response from EC200U-CN.
 * @param response Example: +QMTRECV: 0,0,"EC200_SUB","123456789"
 * @return MQTTMessage Parsed MQTT data.
 */
MQTTMessage parseQMTRECV(const String &response) {
  MQTTMessage msg;
  String payload = response;

  // Remove "+QMTRECV:" prefix if present
  payload.replace("+QMTRECV:", "");
  payload.trim();

  // Split by commas
  int firstComma  = payload.indexOf(',');
  int secondComma = payload.indexOf(',', firstComma + 1);

  // Extract client ID and message ID
  msg.clientId  = payload.substring(0, firstComma).toInt();
  msg.messageId = payload.substring(firstComma + 1, secondComma).toInt();

  // Extract topic (enclosed in quotes)
  int firstQuote = payload.indexOf('"', secondComma);
  int secondQuote = payload.indexOf('"', firstQuote + 1);
  msg.topic = payload.substring(firstQuote + 1, secondQuote);

  // Extract payload (after the topic)
  int thirdQuote = payload.indexOf('"', secondQuote + 1);
  int fourthQuote = payload.indexOf('"', thirdQuote + 1);
  msg.payload = payload.substring(thirdQuote + 1, fourthQuote);

  return msg;
}

void handleMQTTMessage(const String &uartLine) {
  if (uartLine.startsWith("+QMTRECV:")) {
    MQTTMessage msg = parseQMTRECV(uartLine);

    Serial.println("----- MQTT MESSAGE RECEIVED -----");
    Serial.print("Client ID : "); Serial.println(msg.clientId);
    Serial.print("Message ID: "); Serial.println(msg.messageId);
    Serial.print("Topic     : "); Serial.println(msg.topic);
    Serial.print("Payload   : "); Serial.println(msg.payload);
    Serial.println("--------------------------------");
  }
}
