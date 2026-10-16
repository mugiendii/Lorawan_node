#ifndef LORAWAN_MODULE_H
#define LORAWAN_MODULE_H

#include <Arduino.h>
#include <lmic.h>

// Forward declarations for credentials (defined in config.h in src/)
extern const uint8_t APPEUI[8];
extern const uint8_t DEVEUI[8];
extern const uint8_t APPKEY[16];

// Forward declaration for TX_INTERVAL
#ifndef TX_INTERVAL
#define TX_INTERVAL 30
#endif

// Range test data structure
struct RangeTestData {
    uint16_t packetCounter;
    int16_t rssi;
    int8_t snr;
    uint8_t spreadingFactor;
    uint8_t dataRate;
    uint32_t frequency;
    int16_t gatewayRssi;
    float batteryVoltage;
};

// LoRaWAN module class
class LoRaWANModule {
public:
    // Constructor
    LoRaWANModule();

    // Initialization
    void begin();

    // Join network
    void startJoin();

    // Send uplink
    void sendUplink();

    // Check if joined
    bool isJoined();

    // Get device address
    uint32_t getDevAddr();

    // Get packet counter
    uint16_t getPacketCount();

    // Check if radio is busy
    bool isBusy();

    // Get last range test data
    RangeTestData getLastRangeData();

    // Set auto mode
    void setAutoMode(bool enabled);

    // Get auto mode status
    bool isAutoMode();

    // Data rate control (EU868: 0 = SF12 .. 5 = SF7)
    bool setDataRate(uint8_t dr);
    void enableAdr();
    bool isAdrEnabled();
    uint8_t getDataRate();

    // Event callbacks
    void onJoinSuccess(void (*callback)());
    void onJoinFailed(void (*callback)());
    void onTxComplete(void (*callback)(bool success));

    // Process LMIC events (call this in loop)
    void process();

private:
    RangeTestData rangeData;
    uint16_t packetCounter;
    bool autoMode;
    bool joinRequested;
    bool uplinkRequested;
    bool waitingForResponse;

    // Callback function pointers
    void (*joinSuccessCallback)();
    void (*joinFailedCallback)();
    void (*txCompleteCallback)(bool success);

    // Internal methods
    void prepareRangeTestData();
    void buildPayload(uint8_t* payload);

    // Friend function for LMIC callbacks
    friend void onEvent(ev_t ev);
    friend void do_send(osjob_t* j);
};

// Global instance
extern LoRaWANModule lorawan;

#endif // LORAWAN_MODULE_H
