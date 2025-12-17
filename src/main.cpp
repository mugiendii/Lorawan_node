#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "config.h"  // WiFi and LoRaWAN configuration

#ifdef USE_WIFI
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

// Range test data structure
struct RangeTestData {
    uint16_t packetCounter;
    int16_t rssi;
    int8_t snr;
    uint8_t spreadingFactor;
    uint32_t frequency;
    int16_t gatewayRssi;
    float batteryVoltage;
};

// Function prototypes
void do_send(osjob_t* j);
void onEvent(ev_t ev);

// LMIC callbacks
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}

// Pin mapping based on schematic - Wemos D1 Mini with RFM95-868M
// Standard SPI pins (MISO, MOSI, SCK) are used automatically
// SS -> D8 (GPIO15), D1 -> GPIO5, D2 -> GPIO4
const lmic_pinmap lmic_pins = {
    .nss = 15,      // SS/NSS - D8 (GPIO15)
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,  // RESET not connected in schematic
    .dio = {5, 4, LMIC_UNUSED_PIN},  // DIO0=D1(GPIO5), DIO1=D2(GPIO4), DIO2=NC
};

// Variables
static osjob_t sendjob;
static uint16_t packetCounter = 0;
static RangeTestData rangeData;

// Communication functions
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
    Serial.print(F("Battery: "));
    Serial.print(data.batteryVoltage, 2);
    Serial.println(F(" V"));
    Serial.println(F("======================"));
}

// Event handler
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                Serial.print(artKey[i], HEX);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      Serial.print(nwkKey[i], HEX);
              }
              Serial.println();
            }
            // Disable link check validation (automatically enabled during join)
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK) {
              Serial.println(F("Received ack"));
              // Update gateway RSSI/SNR from downlink
              Serial.print(F("Gateway RSSI: "));
              Serial.print(LMIC.rssi);
              Serial.println(F(" dBm"));
              Serial.print(F("Gateway SNR: "));
              Serial.print(LMIC.snr);
              Serial.println(F(" dB"));
            }
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
              Serial.print(F("Data: "));
              for (int i = 0; i < LMIC.dataLen; i++) {
                Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
                Serial.print(" ");
              }
              Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            Serial.println(F("Retrying join in 10 seconds..."));
            // Automatically retry join after delay
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(10), do_send);
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Check if joined
        if (LMIC.devaddr == 0) {
            Serial.println(F("Not joined yet, cannot send data"));
            // Retry join
            LMIC_startJoining();
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(10), do_send);
            return;
        }

        // Prepare range test data
        rangeData.packetCounter = packetCounter++;
        rangeData.rssi = LMIC.rssi;
        rangeData.snr = LMIC.snr;
        rangeData.spreadingFactor = getSf(LMIC.datarate) + 6; // Convert DR to SF
        rangeData.frequency = LMIC.freq;
        rangeData.gatewayRssi = 0; // Will be updated from downlink if available
        rangeData.batteryVoltage = 3.3; // Read from ADC if battery monitoring is implemented

        // Build compact binary payload for LoRaWAN
        // Format: [counter:2][rssi:1][snr:1][sf:1][freq:4][battery:2]
        uint8_t payload[11];
        payload[0] = (rangeData.packetCounter >> 8) & 0xFF;
        payload[1] = rangeData.packetCounter & 0xFF;
        payload[2] = (uint8_t)(rangeData.rssi + 200); // Offset by 200 to fit in uint8
        payload[3] = (uint8_t)(rangeData.snr + 128); // Offset by 128
        payload[4] = rangeData.spreadingFactor;
        payload[5] = (rangeData.frequency >> 24) & 0xFF;
        payload[6] = (rangeData.frequency >> 16) & 0xFF;
        payload[7] = (rangeData.frequency >> 8) & 0xFF;
        payload[8] = rangeData.frequency & 0xFF;
        uint16_t batteryMv = (uint16_t)(rangeData.batteryVoltage * 1000);
        payload[9] = (batteryMv >> 8) & 0xFF;
        payload[10] = batteryMv & 0xFF;

        // Print range test data
        sendRangeTestData(rangeData);

        // Send via LoRaWAN with confirmed uplink (1 = request ACK from gateway)
        // This forces a downlink which contains gateway RSSI/SNR
        LMIC_setTxData2(1, payload, sizeof(payload), 1);  // Last param: 1 = confirmed
        Serial.println(F("Range test packet queued (confirmed)"));

        #ifdef USE_WIFI
        // Optionally send to HTTP server
        sendDataToServer(SERVER_URL, rangeData);
        #endif
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    
    Serial.println(F("   LoRaWAN Range Test Node"));
    
    Serial.println(F("Target: ChirpStack Gateway"));
    Serial.println(F("Region: EU868"));
    Serial.println(F("Radio: RFM95-868M (SX1276)"));
   

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

    

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set clock error for better RX window timing
    // ESP8266 RC oscillator is not very accurate, so increase tolerance
    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Don't set data rate manually - let LMIC use adaptive data rate for join
    // The join process will automatically use appropriate spreading factors
    
    Serial.println(F("LMIC initialized, starting join..."));
    Serial.print(F("LMIC TX Frequency: "));
    Serial.println(LMIC.freq);
    Serial.print(F("LMIC Data Rate: "));
    Serial.println(LMIC.datarate);

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
