#include <Arduino.h>
#include "config.h"
#include <lorawan_module.h>

#ifdef USE_WIFI
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

// Function prototypes
void processSerialCommand(String cmd);
void handleJoinCommand();
void handleTriggerCommand();
void printRangeTestResult(bool success);
void printHelp();

// Callback functions
void onJoinSuccessCallback();
void onJoinFailedCallback();
void onTxCompleteCallback(bool success);

#ifdef USE_WIFI
void setupWiFi(const char* ssid, const char* password);
void sendDataToServer(const char* serverUrl, const RangeTestData& data);
#endif

// WiFi functions
#ifdef USE_WIFI
void setupWiFi(const char* ssid, const char* password) {
    Serial.println(F("Connecting to WiFi..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print(F("WiFi connected! IP: "));
        Serial.println(WiFi.localIP());
    } else {
        Serial.println();
        Serial.println(F("WiFi connection failed!"));
    }
}

void sendDataToServer(const char* serverUrl, const RangeTestData& data) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi not connected, skipping HTTP upload"));
        return;
    }
    WiFiClient client;
    HTTPClient http;
    if (http.begin(client, serverUrl)) {
        http.addHeader("Content-Type", "application/json");
        String payload = "{";
        payload += "\"packet\":" + String(data.packetCounter) + ",";
        payload += "\"rssi\":" + String(data.rssi) + ",";
        payload += "\"snr\":" + String(data.snr) + ",";
        payload += "\"sf\":" + String(data.spreadingFactor) + ",";
        payload += "\"freq\":" + String(data.frequency) + ",";
        payload += "\"gw_rssi\":" + String(data.gatewayRssi) + ",";
        payload += "\"battery\":" + String(data.batteryVoltage, 2);
        payload += "}";
        int httpCode = http.POST(payload);
        if (httpCode > 0) {
            Serial.print(F("HTTP: "));
            Serial.println(httpCode);
        }
        http.end();
    }
}
#endif

// Serial command handlers
void processSerialCommand(String cmd) {
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "JOIN") {
        handleJoinCommand();
    } else if (cmd == "TRIGGER") {
        handleTriggerCommand();
    } else if (cmd == "AUTO") {
        lorawan.setAutoMode(true);
        Serial.println(F("AUTO mode enabled - continuous transmission"));
    } else if (cmd == "MANUAL") {
        lorawan.setAutoMode(false);
        Serial.println(F("MANUAL mode enabled - use JOIN and TRIGGER commands"));
    } else if (cmd == "STATUS") {
        Serial.print(F("Mode: "));
        Serial.println(lorawan.isAutoMode() ? F("AUTO") : F("MANUAL"));
        Serial.print(F("Joined: "));
        Serial.println(lorawan.isJoined() ? F("YES") : F("NO"));
        if (lorawan.isJoined()) {
            Serial.print(F("DevAddr: "));
            Serial.println(lorawan.getDevAddr(), HEX);
        }
        Serial.print(F("Packets sent: "));
        Serial.println(lorawan.getPacketCount());
    } else if (cmd == "HELP") {
        printHelp();
    } else {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("Type HELP for available commands"));
    }
}

void handleJoinCommand() {
    lorawan.startJoin();
}

void handleTriggerCommand() {
    lorawan.sendUplink();
}

void printHelp() {
    Serial.println(F("Available commands:"));
    Serial.println(F("  JOIN    - Start OTAA join procedure"));
    Serial.println(F("  TRIGGER - Send uplink and get range test result"));
    Serial.println(F("  AUTO    - Enable automatic transmission mode"));
    Serial.println(F("  MANUAL  - Enable manual command mode"));
    Serial.println(F("  STATUS  - Show device status"));
    Serial.println(F("  HELP    - Show this help"));
}

void printRangeTestResult(bool success) {
    if (!success) {
        Serial.println(F("RANGE TEST: FAILED"));
        Serial.println(F("No response received"));
        Serial.println(F("Status: OUT OF RANGE"));
        return;
    }

    RangeTestData data = lorawan.getLastRangeData();

  
    Serial.println(F("RANGE TEST: SUCCESS"));
    Serial.print(F("Packet #: "));
    Serial.println(data.packetCounter);

    // Uplink (device -> gateway) metrics
    Serial.println(F("\n--- Uplink Quality ---"));
    Serial.print(F("RSSI: "));
    Serial.print(data.rssi);
    Serial.println(F(" dBm"));
    Serial.print(F("SNR: "));
    Serial.print(data.snr);
    Serial.println(F(" dB"));
    Serial.print(F("SF: "));
    Serial.println(data.spreadingFactor);
    Serial.print(F("Freq: "));
    Serial.print(data.frequency / 1000000.0, 1);
    Serial.println(F(" MHz"));

    // Downlink (gateway -> device) metrics
    Serial.println(F("\n--- Downlink Quality ---"));
    Serial.print(F("Gateway RSSI: "));
    Serial.print(LMIC.rssi);
    Serial.println(F(" dBm"));
    Serial.print(F("Gateway SNR: "));
    Serial.print(LMIC.snr);
    Serial.println(F(" dB"));

    // Distance estimate (rough calculation based on RSSI)
    int avgRssi = (data.rssi + LMIC.rssi) / 2;
    float distance = 0;
    if (avgRssi > -50) distance = 0.01;  // < 50m
    else if (avgRssi > -70) distance = 0.1;   // 50-200m
    else if (avgRssi > -90) distance = 0.5;   // 200-500m
    else if (avgRssi > -110) distance = 2.0;  // 500m-2km
    else distance = 5.0;  // > 2km

    Serial.println(F("\n--- Estimated Range ---"));
    Serial.print(F("Distance: ~"));
    Serial.print(distance, 2);
    Serial.println(F(" km"));
    Serial.print(F("Link Quality: "));
    if (avgRssi > -70) Serial.println(F("EXCELLENT"));
    else if (avgRssi > -90) Serial.println(F("GOOD"));
    else if (avgRssi > -110) Serial.println(F("FAIR"));
    else Serial.println(F("POOR"));
    #ifdef USE_WIFI
    // Send to HTTP server
    sendDataToServer(SERVER_URL, data);
    #endif
}

// Callback implementations
void onJoinSuccessCallback() {
    Serial.println(F("JOIN: SUCCESS"));
    Serial.println(F("Network joined successfully!"));
    Serial.println(F("Ready to send uplinks."));
    Serial.println(F("Use TRIGGER command to test range."));
}

void onJoinFailedCallback() {
    Serial.println(F("JOIN: FAILED"));
    Serial.println(F("Unable to join network"));
    Serial.println(F("Check credentials and gateway"));
}

void onTxCompleteCallback(bool success) {
    printRangeTestResult(success);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("   LoRaWAN Range Test Node"));
    
    Serial.println(F("Target: ChirpStack Gateway"));
    Serial.println(F("Region: EU868"));
    Serial.println(F("Radio: RFM95-868M (SX1276)"));
    Serial.println(F(""));
    Serial.println(F("Serial Commands Available:"));
    Serial.println(F("  JOIN    - Start OTAA join procedure"));
    Serial.println(F("  TRIGGER - Send uplink and test range"));
    Serial.println(F("  AUTO    - Enable automatic transmission"));
    Serial.println(F("  MANUAL  - Enable manual command mode"));
    Serial.println(F("  STATUS  - Show device status"));
    Serial.println(F("  HELP    - Show help"));
    Serial.println(F(""));
    Serial.println(F("Starting in AUTO mode..."));

    #ifdef USE_WIFI
    // Setup WiFi for HTTP logging
    setupWiFi(WIFI_SSID, WIFI_PASSWORD);
    #endif

    // Print credentials for verification
    Serial.print(F("APPEUI: "));
    for (int i = 0; i < 8; i++) {
        Serial.print(pgm_read_byte(&APPEUI[i]), HEX);
        Serial.print(" ");
    }
    Serial.println();

    Serial.print(F("DEVEUI: "));
    for (int i = 0; i < 8; i++) {
        Serial.print(pgm_read_byte(&DEVEUI[i]), HEX);
        Serial.print(" ");
    }
    Serial.println();

    // Initialize LoRaWAN module
    lorawan.begin();

    // Register callbacks
    lorawan.onJoinSuccess(onJoinSuccessCallback);
    lorawan.onJoinFailed(onJoinFailedCallback);
    lorawan.onTxComplete(onTxCompleteCallback);

    // Start join process in auto mode
    Serial.println(F("Starting join process..."));
    lorawan.startJoin();
}

void loop() {
    // Process LoRaWAN events
    lorawan.process();

    // Handle serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        processSerialCommand(command);
    }
}
