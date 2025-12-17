// communication.cpp - Range Test Communication Library Implementation

#include "communication.h"

// WiFi setup
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

// Check if WiFi is connected
bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Send range test data via serial (for logging/debugging)
void sendRangeTestData(const RangeTestData& data) {
    Serial.println(F("=== Range Test Data ==="));
    Serial.print(F("Packet: "));
    Serial.println(data.packetCounter);
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
    Serial.print(F("Gateway RSSI: "));
    Serial.print(data.gatewayRssi);
    Serial.println(F(" dBm"));
    Serial.print(F("Battery: "));
    Serial.print(data.batteryVoltage, 2);
    Serial.println(F(" V"));
    Serial.println(F("======================"));
}

// Send range test data to HTTP server
void sendDataToServer(const char* serverUrl, const RangeTestData& data) {
    if (!isWiFiConnected()) {
        Serial.println(F("WiFi not connected, skipping HTTP upload"));
        return;
    }

    WiFiClient client;
    HTTPClient http;

    if (http.begin(client, serverUrl)) {
        http.addHeader("Content-Type", "application/json");

        // Build JSON payload
        String payload = "{";
        payload += "\"packet\":" + String(data.packetCounter) + ",";
        payload += "\"rssi\":" + String(data.rssi) + ",";
        payload += "\"snr\":" + String(data.snr) + ",";
        payload += "\"sf\":" + String(data.spreadingFactor) + ",";
        payload += "\"freq\":" + String(data.frequency) + ",";
        payload += "\"gw_rssi\":" + String(data.gatewayRssi) + ",";
        payload += "\"battery\":" + String(data.batteryVoltage, 2);
        payload += "}";

        Serial.print(F("Sending to server: "));
        Serial.println(payload);

        int httpCode = http.POST(payload);

        if (httpCode > 0) {
            Serial.print(F("HTTP Response: "));
            Serial.println(httpCode);
            if (httpCode == HTTP_CODE_OK) {
                String response = http.getString();
                Serial.println(response);
            }
        } else {
            Serial.print(F("HTTP Error: "));
            Serial.println(http.errorToString(httpCode));
        }

        http.end();
    } else {
        Serial.println(F("HTTP connection failed"));
    }
}
