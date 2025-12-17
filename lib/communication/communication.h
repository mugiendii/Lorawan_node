// communication.h - Range Test Communication Library

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Range test data structure
struct RangeTestData {
    uint16_t packetCounter;
    int16_t rssi;           // RSSI at device
    int8_t snr;             // SNR at device
    uint8_t spreadingFactor;
    uint32_t frequency;
    int16_t gatewayRssi;    // RSSI at gateway (from downlink)
    float batteryVoltage;
};

// Function prototypes
void setupWiFi(const char* ssid, const char* password);
void sendRangeTestData(const RangeTestData& data);
void sendDataToServer(const char* serverUrl, const RangeTestData& data);
bool isWiFiConnected();

#endif