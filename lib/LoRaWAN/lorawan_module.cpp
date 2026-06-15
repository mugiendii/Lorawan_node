#include "lorawan_module.h"
#include <hal/hal.h>

// LMIC callbacks - use extern declarations from header
void os_getArtEui(u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui(u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey(u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// Pin mapping based on schematic - Wemos D1 Mini with RFM95-868M
const lmic_pinmap lmic_pins = {
    .nss = 15,      // SS/NSS - D8 (GPIO15)
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,  // RESET not connected in schematic
    .dio = {5, 4, LMIC_UNUSED_PIN},  // DIO0=D1(GPIO5), DIO1=D2(GPIO4), DIO2=NC
};

// Global instance
LoRaWANModule lorawan;

// Static job for LMIC
static osjob_t sendjob;

// Forward declaration
void do_send(osjob_t* j);
void onEvent(ev_t ev);

// Constructor
LoRaWANModule::LoRaWANModule() {
    packetCounter = 0;
    autoMode = true;
    joinRequested = false;
    uplinkRequested = false;
    waitingForResponse = false;
    joinSuccessCallback = nullptr;
    joinFailedCallback = nullptr;
    txCompleteCallback = nullptr;
}

// Initialize LoRaWAN
void LoRaWANModule::begin() {
    // LMIC init
    os_init();
    // Reset the MAC state
    LMIC_reset();

    // Set clock error for better RX window timing
    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    Serial.println(F("LoRaWAN module initialized"));
    Serial.print(F("LMIC TX Frequency: "));
    Serial.println(LMIC.freq);
    Serial.print(F("LMIC Data Rate: "));
    Serial.println(LMIC.datarate);
}

// Start OTAA join
void LoRaWANModule::startJoin() {
    if (isJoined()) {
        Serial.println(F("Already joined!"));
        return;
    }

    if (isBusy()) {
        Serial.println(F("Radio busy, please wait..."));
        return;
    }

    Serial.println(F("Starting OTAA join..."));
    joinRequested = true;
    LMIC_startJoining();
}

// Send uplink
void LoRaWANModule::sendUplink() {
    if (!isJoined()) {
        Serial.println(F("ERROR: Not joined to network!"));
        return;
    }

    if (isBusy()) {
        Serial.println(F("ERROR: Radio busy"));
        return;
    }

    if (waitingForResponse) {
        Serial.println(F("ERROR: Already waiting for response"));
        return;
    }

    Serial.println(F("Sending uplink..."));
    uplinkRequested = true;
    waitingForResponse = true;
    do_send(&sendjob);
}

// Check if joined
bool LoRaWANModule::isJoined() {
    return LMIC.devaddr != 0;
}

// Get device address
uint32_t LoRaWANModule::getDevAddr() {
    return LMIC.devaddr;
}

// Get packet counter
uint16_t LoRaWANModule::getPacketCount() {
    return packetCounter;
}

// Check if radio is busy
bool LoRaWANModule::isBusy() {
    return (LMIC.opmode & OP_TXRXPEND) != 0;
}

// Get last range test data
RangeTestData LoRaWANModule::getLastRangeData() {
    return rangeData;
}

// Set auto mode
void LoRaWANModule::setAutoMode(bool enabled) {
    autoMode = enabled;
    if (enabled) {
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(5), do_send);
    }
}

// Get auto mode status
bool LoRaWANModule::isAutoMode() {
    return autoMode;
}

// Set callbacks
void LoRaWANModule::onJoinSuccess(void (*callback)()) {
    joinSuccessCallback = callback;
}

void LoRaWANModule::onJoinFailed(void (*callback)()) {
    joinFailedCallback = callback;
}

void LoRaWANModule::onTxComplete(void (*callback)(bool success)) {
    txCompleteCallback = callback;
}

// Process LMIC events
void LoRaWANModule::process() {
    os_runloop_once();
}

// Prepare range test data
void LoRaWANModule::prepareRangeTestData() {
    rangeData.packetCounter = packetCounter++;
    rangeData.rssi = LMIC.rssi;
    rangeData.snr = LMIC.snr;
    rangeData.spreadingFactor = getSf(LMIC.datarate) + 6;
    rangeData.frequency = LMIC.freq;
    rangeData.gatewayRssi = 0;
    rangeData.batteryVoltage = 3.3; // TODO: Read from ADC
}

// Build payload
void LoRaWANModule::buildPayload(uint8_t* payload) {
    // Format: [counter:2][rssi:1][snr:1][sf:1][freq:4][battery:2]
    payload[0] = (rangeData.packetCounter >> 8) & 0xFF;
    payload[1] = rangeData.packetCounter & 0xFF;
    payload[2] = (uint8_t)(rangeData.rssi + 200);
    payload[3] = (uint8_t)(rangeData.snr + 128);
    payload[4] = rangeData.spreadingFactor;
    payload[5] = (rangeData.frequency >> 24) & 0xFF;
    payload[6] = (rangeData.frequency >> 16) & 0xFF;
    payload[7] = (rangeData.frequency >> 8) & 0xFF;
    payload[8] = rangeData.frequency & 0xFF;
    uint16_t batteryMv = (uint16_t)(rangeData.batteryVoltage * 1000);
    payload[9] = (batteryMv >> 8) & 0xFF;
    payload[10] = batteryMv & 0xFF;
}

// LMIC event handler
void onEvent(ev_t ev) {
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
                Serial.print(F("netid: "));
                Serial.println(netid, DEC);
                Serial.print(F("devaddr: "));
                Serial.println(devaddr, HEX);
                Serial.print(F("AppSKey: "));
                for (size_t i = 0; i < sizeof(artKey); ++i) {
                    if (i != 0) Serial.print("-");
                    Serial.print(artKey[i], HEX);
                }
                Serial.println();
                Serial.print(F("NwkSKey: "));
                for (size_t i = 0; i < sizeof(nwkKey); ++i) {
                    if (i != 0) Serial.print("-");
                    Serial.print(nwkKey[i], HEX);
                }
                Serial.println();
            }
            LMIC_setLinkCheckMode(0);

            // Call user callback
            if (!lorawan.autoMode && lorawan.joinRequested) {
                lorawan.joinRequested = false;
                if (lorawan.joinSuccessCallback) {
                    lorawan.joinSuccessCallback();
                }
            }
            break;

        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            if (!lorawan.autoMode && lorawan.joinRequested) {
                lorawan.joinRequested = false;
                if (lorawan.joinFailedCallback) {
                    lorawan.joinFailedCallback();
                }
            }
            break;

        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            if (!lorawan.autoMode && lorawan.joinRequested) {
                lorawan.joinRequested = false;
                if (lorawan.joinFailedCallback) {
                    lorawan.joinFailedCallback();
                }
            }
            break;

        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));

            if (LMIC.txrxFlags & TXRX_ACK) {
                Serial.println(F("Received ack"));
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

            // Handle TX complete
            if (!lorawan.autoMode && lorawan.uplinkRequested && lorawan.waitingForResponse) {
                bool success = (LMIC.txrxFlags & TXRX_ACK) || (LMIC.dataLen > 0);
                lorawan.uplinkRequested = false;
                lorawan.waitingForResponse = false;

                if (lorawan.txCompleteCallback) {
                    lorawan.txCompleteCallback(success);
                }
            }

            // Schedule next transmission in auto mode
            if (lorawan.autoMode) {
                os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            }
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
            break;

        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            Serial.println(F("Retrying join in 10 seconds..."));
            if (lorawan.autoMode) {
                os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(10), do_send);
            }
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned)ev);
            break;
    }
}

// Send function
void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        return;
    }

    if (LMIC.devaddr == 0) {
        Serial.println(F("Not joined yet, cannot send data"));
        if (lorawan.autoMode) {
            LMIC_startJoining();
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(10), do_send);
        }
        return;
    }

    // Prepare range test data
    lorawan.prepareRangeTestData();

    // Build payload
    uint8_t payload[11];
    lorawan.buildPayload(payload);

    // Print range test data
    RangeTestData data = lorawan.getLastRangeData();
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

    // Send via LoRaWAN with confirmed uplink
    LMIC_setTxData2(1, payload, sizeof(payload), 1);
    Serial.println(F("Range test packet queued (confirmed)"));
}
