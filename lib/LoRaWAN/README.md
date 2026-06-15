# LoRaWAN Library

A clean, reusable LoRaWAN module for ESP8266 with SX1276/RFM95 radio using the MCCI LMIC library.

## Features

- **Easy OTAA Join**: Simple API for Over-The-Air Activation
- **Uplink Transmission**: Confirmed and unconfirmed uplinks
- **Range Testing**: Built-in range test data collection
- **Auto/Manual Modes**: Automatic or command-driven operation
- **Event Callbacks**: Hook into join success/failure and transmission events
- **Non-blocking**: Uses LMIC's event-driven architecture

## Installation

This library is designed for PlatformIO. Simply place it in your project's `lib/` folder and include it:

```cpp
#include <lorawan_module.h>
```

## Dependencies

- MCCI LoRaWAN LMIC library
- Arduino framework
- config.h file with LoRaWAN credentials

## Quick Start

```cpp
#include <Arduino.h>
#include <lorawan_module.h>

void onJoined() {
    Serial.println("Joined network!");
}

void onTxComplete(bool success) {
    Serial.println(success ? "TX Success!" : "TX Failed");
}

void setup() {
    Serial.begin(115200);

    // Initialize LoRaWAN module
    lorawan.begin();

    // Set callbacks
    lorawan.onJoinSuccess(onJoined);
    lorawan.onTxComplete(onTxComplete);

    // Start OTAA join
    lorawan.startJoin();
}

void loop() {
    // Process LoRaWAN events
    lorawan.process();

    // Send uplink when joined
    if (lorawan.isJoined() && !lorawan.isBusy()) {
        lorawan.sendUplink();
        delay(30000); // Wait 30 seconds
    }
}
```

## API Reference

### Initialization

#### `void begin()`
Initialize the LoRaWAN module and LMIC stack.

**Example:**
```cpp
lorawan.begin();
```

---

### Join Management

#### `void startJoin()`
Start OTAA (Over-The-Air Activation) join procedure.

**Example:**
```cpp
lorawan.startJoin();
```

---

### Data Transmission

#### `void sendUplink()`
Send a confirmed uplink packet with range test data.

**Returns:** Nothing (asynchronous)

**Example:**
```cpp
if (lorawan.isJoined()) {
    lorawan.sendUplink();
}
```

---

### Status Queries

#### `bool isJoined()`
Check if device has joined the network.

**Returns:** `true` if joined, `false` otherwise

**Example:**
```cpp
if (lorawan.isJoined()) {
    Serial.println("Connected!");
}
```

#### `bool isBusy()`
Check if radio is currently busy with TX/RX operation.

**Returns:** `true` if busy, `false` if idle

**Example:**
```cpp
if (!lorawan.isBusy()) {
    lorawan.sendUplink();
}
```

#### `uint32_t getDevAddr()`
Get the device address assigned during join.

**Returns:** 32-bit device address (0 if not joined)

**Example:**
```cpp
Serial.println(lorawan.getDevAddr(), HEX);
```

#### `uint16_t getPacketCount()`
Get the number of packets sent.

**Returns:** Packet counter

**Example:**
```cpp
Serial.print("Packets sent: ");
Serial.println(lorawan.getPacketCount());
```

---

### Range Test Data

#### `RangeTestData getLastRangeData()`
Get the last collected range test data.

**Returns:** `RangeTestData` struct

**Example:**
```cpp
RangeTestData data = lorawan.getLastRangeData();
Serial.print("RSSI: ");
Serial.println(data.rssi);
```

**RangeTestData Structure:**
```cpp
struct RangeTestData {
    uint16_t packetCounter;      // Packet sequence number
    int16_t rssi;                // RSSI in dBm
    int8_t snr;                  // SNR in dB
    uint8_t spreadingFactor;     // SF7-SF12
    uint32_t frequency;          // Frequency in Hz
    int16_t gatewayRssi;         // Gateway RSSI (from downlink)
    float batteryVoltage;        // Battery voltage in V
};
```

---

### Mode Control

#### `void setAutoMode(bool enabled)`
Enable or disable automatic transmission mode.

**Parameters:**
- `enabled`: `true` for auto mode, `false` for manual mode

**Example:**
```cpp
lorawan.setAutoMode(true);  // Auto mode
lorawan.setAutoMode(false); // Manual mode
```

#### `bool isAutoMode()`
Check if auto mode is enabled.

**Returns:** `true` if auto mode, `false` if manual mode

**Example:**
```cpp
if (lorawan.isAutoMode()) {
    Serial.println("Auto mode active");
}
```

---

### Event Processing

#### `void process()`
Process LMIC events. **Must be called in loop()**.

**Example:**
```cpp
void loop() {
    lorawan.process();
}
```

---

### Callbacks

#### `void onJoinSuccess(void (*callback)())`
Register callback for successful join.

**Parameters:**
- `callback`: Function to call when join succeeds

**Example:**
```cpp
void onJoined() {
    Serial.println("Joined!");
}

lorawan.onJoinSuccess(onJoined);
```

#### `void onJoinFailed(void (*callback)())`
Register callback for failed join.

**Parameters:**
- `callback`: Function to call when join fails

**Example:**
```cpp
void onJoinFail() {
    Serial.println("Join failed!");
}

lorawan.onJoinFailed(onJoinFail);
```

#### `void onTxComplete(void (*callback)(bool success))`
Register callback for transmission completion.

**Parameters:**
- `callback`: Function to call when TX completes
  - `success`: `true` if ACK received or downlink present, `false` otherwise

**Example:**
```cpp
void onTxDone(bool success) {
    if (success) {
        Serial.println("TX successful!");
    } else {
        Serial.println("TX failed or no response");
    }
}

lorawan.onTxComplete(onTxDone);
```

---

## Configuration

Create a `config.h` file in your `src/` folder:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// Transmission interval (seconds)
#define TX_INTERVAL 30

// LoRaWAN Credentials (OTAA)
// APPEUI and DEVEUI in LSB format (reversed)
static const uint8_t PROGMEM APPEUI[8] = { 0x00, 0x00, ... };
static const uint8_t PROGMEM DEVEUI[8] = { 0x00, 0x00, ... };

// APPKEY in MSB format (same as shown in console)
static const uint8_t PROGMEM APPKEY[16] = { 0x00, 0x00, ... };

#endif
```

## Operating Modes

### Auto Mode
- Automatically joins on startup
- Sends periodic uplinks every `TX_INTERVAL` seconds
- Auto-retry on join failure

### Manual Mode
- Requires explicit `startJoin()` call
- `sendUplink()` must be called manually
- No automatic retries

## Payload Format

The library sends a compact 11-byte binary payload:

| Bytes | Field | Format |
|-------|-------|--------|
| 0-1   | Packet Counter | uint16_t |
| 2     | RSSI | uint8_t (offset +200) |
| 3     | SNR | uint8_t (offset +128) |
| 4     | Spreading Factor | uint8_t (7-12) |
| 5-8   | Frequency | uint32_t (Hz) |
| 9-10  | Battery Voltage | uint16_t (mV) |

## Example: Manual Control

```cpp
void setup() {
    lorawan.begin();
    lorawan.setAutoMode(false);  // Manual mode

    lorawan.onJoinSuccess([]() {
        Serial.println("Ready to send!");
    });

    lorawan.startJoin();
}

void loop() {
    lorawan.process();

    // Send on button press
    if (digitalRead(BUTTON_PIN) == LOW && lorawan.isJoined()) {
        lorawan.sendUplink();
        delay(100); // Debounce
    }
}
```

## Example: Auto Mode with Data Logging

```cpp
void setup() {
    lorawan.begin();
    lorawan.setAutoMode(true);  // Auto mode

    lorawan.onTxComplete([](bool success) {
        RangeTestData data = lorawan.getLastRangeData();

        Serial.print("Packet: ");
        Serial.print(data.packetCounter);
        Serial.print(" | RSSI: ");
        Serial.print(data.rssi);
        Serial.print(" | Status: ");
        Serial.println(success ? "OK" : "FAIL");
    });

    lorawan.startJoin();
}

void loop() {
    lorawan.process();
}
```

## Troubleshooting

**Join fails repeatedly:**
- Check credentials (APPEUI, DEVEUI, APPKEY)
- Verify LSB/MSB format
- Check gateway is online
- Verify antenna connection

**Radio busy errors:**
- Wait for previous transmission to complete
- Check `isBusy()` before sending

**No callbacks triggered:**
- Make sure `process()` is called in `loop()`
- Don't use blocking delays

## License

This library is part of the Range Test project and uses the MCCI LMIC library.

## Version

1.0.0
