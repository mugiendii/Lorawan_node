#include <Arduino.h>
#include "config.h"
#include <lorawan_module.h>

#ifdef USE_WIFI
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
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

// Data rate sweep
extern bool sweepActive;
void startSweep();
void finishSweep(int recommended, int fallback);

#ifdef USE_WIFI
void setupWiFi();
void checkWiFi();
void sendDataToServer(const char* serverUrl, const RangeTestData& data);
#endif

// WiFi functions
#ifdef USE_WIFI
const __FlashStringHelper* wifiStatusText(wl_status_t status) {
    switch (status) {
        case WL_CONNECTED:       return F("connected");
        case WL_NO_SSID_AVAIL:   return F("network not found (check name, and that the hotspot is on 2.4 GHz)");
        case WL_WRONG_PASSWORD:  return F("wrong password");
        case WL_CONNECT_FAILED:  return F("connection refused (check WPA2, not WPA3-only)");
        case WL_IDLE_STATUS:     return F("still connecting");
        case WL_DISCONNECTED:    return F("disconnected");
        default:                 return F("unknown");
    }
}

ESP8266WiFiMulti wifiMulti;
const size_t WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// List what the radio can hear, marking the configured networks
void printVisibleNetworks() {
    int found = WiFi.scanNetworks();
    Serial.print(F("Networks visible: "));
    Serial.println(found);
    for (int i = 0; i < found; i++) {
        bool known = false;
        for (size_t n = 0; n < WIFI_NETWORK_COUNT; n++) {
            if (WiFi.SSID(i) == WIFI_NETWORKS[n].ssid) known = true;
        }
        Serial.print(known ? F("  * ") : F("    "));
        Serial.print(WiFi.SSID(i));
        Serial.print(F("  ch"));
        Serial.print(WiFi.channel(i));
        Serial.print(F("  "));
        Serial.print(WiFi.RSSI(i));
        Serial.println(F(" dBm"));
    }
    WiFi.scanDelete();
}

void setupWiFi() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    Serial.print(F("WiFi networks configured: "));
    for (size_t n = 0; n < WIFI_NETWORK_COUNT; n++) {
        wifiMulti.addAP(WIFI_NETWORKS[n].ssid, WIFI_NETWORKS[n].password);
        if (n) Serial.print(F(", "));
        Serial.print(WIFI_NETWORKS[n].ssid);
    }
    Serial.println();

    // Joins the strongest configured network that is in range
    if (wifiMulti.run(20000) == WL_CONNECTED) {
        Serial.print(F("WiFi connected to \""));
        Serial.print(WiFi.SSID());
        Serial.print(F("\"! IP: "));
        Serial.print(WiFi.localIP());
        Serial.print(F("  RSSI: "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));
    } else {
        Serial.println(F("WiFi not connected: none of the configured networks answered"));
        Serial.println(F("(* = configured network; missing means the device cannot see it)"));
        printVisibleNetworks();
        Serial.println(F("Will keep retrying in the background"));
    }
}

// Report WiFi changes and retry the network list while disconnected.
// Retries only run while the LoRa radio is idle: a scan blocks for a few seconds.
void checkWiFi() {
    static wl_status_t lastStatus = WL_IDLE_STATUS;
    static unsigned long lastRetry = 0;
    wl_status_t status = WiFi.status();

    if (status != lastStatus) {
        lastStatus = status;
        if (status == WL_CONNECTED) {
            Serial.print(F("WiFi connected to \""));
            Serial.print(WiFi.SSID());
            Serial.print(F("\"! IP: "));
            Serial.println(WiFi.localIP());
        } else {
            Serial.print(F("WiFi: "));
            Serial.println(wifiStatusText(status));
        }
    }

    if (status != WL_CONNECTED && !lorawan.isBusy() && millis() - lastRetry > 30000) {
        lastRetry = millis();
        wifiMulti.run(5000);
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
        payload += "\"dr\":" + String(data.dataRate) + ",";
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
        Serial.print(F("Data rate: DR"));
        Serial.print(lorawan.getDataRate());
        Serial.print(F(" (SF"));
        Serial.print(12 - lorawan.getDataRate());
        Serial.print(F("), ADR "));
        Serial.println(lorawan.isAdrEnabled() ? F("on") : F("off"));
    } else if (cmd.startsWith("DR ")) {
        int dr = cmd.substring(3).toInt();
        if (cmd.length() > 3 && isDigit(cmd[3]) && lorawan.setDataRate(dr)) {
            Serial.print(F("Data rate set to DR"));
            Serial.print(dr);
            Serial.print(F(" (SF"));
            Serial.print(12 - dr);
            Serial.println(F("), ADR off"));
        } else {
            Serial.println(F("Usage: DR <0-5>  (0 = SF12 longest range, 5 = SF7 fastest)"));
        }
    } else if (cmd == "SWEEP") {
        startSweep();
    } else if (cmd == "STOP") {
        if (sweepActive) finishSweep(-1, -1);
    } else if (cmd == "ADR") {
        lorawan.enableAdr();
        Serial.println(F("ADR on - network server chooses the data rate"));
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
    Serial.println(F("  DR <n>  - Fix data rate 0-5 (0 = SF12 .. 5 = SF7), ADR off"));
    Serial.println(F("  ADR     - Let the network choose the data rate"));
    Serial.println(F("  SWEEP   - Test DR5..DR0 here and recommend a data rate"));
    Serial.println(F("  STOP    - Stop a running sweep"));
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

    // Measured by the device on the gateway's ACK (downlink)
    Serial.print(F("RSSI: "));
    Serial.print(data.rssi);
    Serial.println(F(" dBm"));
    Serial.print(F("SNR: "));
    Serial.print(data.snr);
    Serial.println(F(" dB"));
    Serial.print(F("SF: "));
    Serial.println(data.spreadingFactor);
    Serial.print(F("DR: "));
    Serial.println(data.dataRate);
    Serial.print(F("Freq: "));
    Serial.print(data.frequency / 1000000.0, 1);
    Serial.println(F(" MHz"));

    Serial.print(F("Link Quality: "));
    if (data.rssi > -70) Serial.println(F("EXCELLENT"));
    else if (data.rssi > -90) Serial.println(F("GOOD"));
    else if (data.rssi > -110) Serial.println(F("FAIR"));
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

// ---------- Data rate sweep ----------
// Tries DR5 (SF7) down to DR0 (SF12) and stops at the first data rate where every
// uplink is ACKed with at least SWEEP_MARGIN_DB of SNR margin.
struct SweepStep {
    uint8_t acks;
    int snrTotal;
    int rssiTotal;
};

bool sweepActive = false;
bool sweepSendNext = false;
int sweepDr = 5;
uint8_t sweepTry = 0;
SweepStep sweepSteps[6];
bool sweepWasAuto = false;
bool sweepWasAdr = true;
uint8_t sweepPrevDr = 0;

// Lowest SNR (dB) the SX127x can decode at each spreading factor, SF7..SF12
float requiredSnr(uint8_t sf) {
    static const float limits[] = { -7.5, -10, -12.5, -15, -17.5, -20 };
    return limits[sf - 7];
}

void startSweep() {
    if (!lorawan.isJoined()) {
        Serial.println(F("SWEEP: join the network first"));
        return;
    }
    if (sweepActive) {
        Serial.println(F("SWEEP: already running"));
        return;
    }
    sweepWasAuto = lorawan.isAutoMode();
    sweepWasAdr = lorawan.isAdrEnabled();
    sweepPrevDr = lorawan.getDataRate();
    lorawan.setAutoMode(false);
    memset(sweepSteps, 0, sizeof(sweepSteps));
    sweepDr = 5;
    sweepTry = 0;
    sweepActive = true;
    sweepSendNext = true;
    Serial.println(F("DR SWEEP: START"));
    Serial.print(F("Margin needed: "));
    Serial.print(SWEEP_MARGIN_DB);
    Serial.print(F(" dB, uplinks per DR: "));
    Serial.println(SWEEP_TRIES);
    Serial.println(F("(slower data rates wait for the 1% duty cycle; the full sweep can take minutes)"));
}

void finishSweep(int recommended, int fallback) {
    sweepActive = false;
    sweepSendNext = false;

    if (recommended >= 0) {
        Serial.print(F("DR SWEEP: RECOMMENDED DR"));
        Serial.print(recommended);
        Serial.print(F(" (SF"));
        Serial.print(12 - recommended);
        Serial.println(F(")"));
    } else if (fallback >= 0) {
        Serial.print(F("DR SWEEP: RECOMMENDED DR"));
        Serial.print(fallback);
        Serial.print(F(" (SF"));
        Serial.print(12 - fallback);
        Serial.println(F(") LOW MARGIN"));
        Serial.println(F("Every ACK arrived but below the margin; expect losses in bad conditions"));
    } else {
        Serial.println(F("DR SWEEP: NO COVERAGE"));
        Serial.println(F("No data rate got every ACK at this location"));
    }

    // Put the device back the way it was
    if (sweepWasAdr) lorawan.enableAdr();
    else lorawan.setDataRate(sweepPrevDr);
    lorawan.setAutoMode(sweepWasAuto);
    Serial.println(F("DR SWEEP: END"));
}

void sweepRecord(bool success) {
    SweepStep& step = sweepSteps[sweepDr];
    if (success) {
        RangeTestData data = lorawan.getLastRangeData();
        step.acks++;
        step.snrTotal += data.snr;
        step.rssiTotal += data.rssi;
    }
    sweepTry++;
    if (sweepTry < SWEEP_TRIES) {
        sweepSendNext = true;
        return;
    }

    // All tries at this data rate are done
    uint8_t sf = 12 - sweepDr;
    Serial.print(F("SWEEP DR"));
    Serial.print(sweepDr);
    Serial.print(F(" SF"));
    Serial.print(sf);
    Serial.print(F(": "));
    Serial.print(step.acks);
    Serial.print(F("/"));
    Serial.print(SWEEP_TRIES);
    Serial.print(F(" ACK"));
    bool good = false;
    if (step.acks > 0) {
        float snr = (float)step.snrTotal / step.acks;
        float margin = snr - requiredSnr(sf);
        Serial.print(F(", RSSI "));
        Serial.print(step.rssiTotal / step.acks);
        Serial.print(F(" dBm, SNR "));
        Serial.print(snr, 1);
        Serial.print(F(" dB, margin "));
        Serial.print(margin, 1);
        Serial.print(F(" dB"));
        good = step.acks == SWEEP_TRIES && margin >= SWEEP_MARGIN_DB;
    }
    Serial.println();

    if (good) {
        finishSweep(sweepDr, -1);
        return;
    }
    if (sweepDr == 0) {
        // Nothing met the margin: fall back to the fastest data rate that got every ACK
        int fallback = -1;
        for (int dr = 5; dr >= 0 && fallback < 0; dr--) {
            if (sweepSteps[dr].acks == SWEEP_TRIES) fallback = dr;
        }
        finishSweep(-1, fallback);
        return;
    }
    sweepDr--;
    sweepTry = 0;
    sweepSendNext = true;
}

void onTxCompleteCallback(bool success) {
    printRangeTestResult(success);
    if (sweepActive) sweepRecord(success);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("   LoRaWAN Range Test Node"));
    // "Power On" / "External System" are normal; "Hardware Watchdog" or repeated
    // resets during TX usually mean the supply dips when the radio transmits
    Serial.print(F("Reset reason: "));
    Serial.println(ESP.getResetReason());
    
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
    setupWiFi();
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

    // Next sweep uplink once the radio is free (LMIC waits out the duty cycle itself)
    if (sweepSendNext && !lorawan.isBusy()) {
        sweepSendNext = false;
        lorawan.setDataRate(sweepDr);
        lorawan.sendUplink();
    }

    // Apply the configured data rate once after joining
    static bool wasJoined = false;
    if (!wasJoined && lorawan.isJoined()) {
        wasJoined = true;
        if (DEFAULT_DR >= 0 && DEFAULT_DR <= 5) {
            lorawan.setDataRate(DEFAULT_DR);
            Serial.print(F("Data rate fixed at DR"));
            Serial.println(DEFAULT_DR);
        }
    }

    #ifdef USE_WIFI
    checkWiFi();
    #endif

    // Handle serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        processSerialCommand(command);
    }
}
