# LoRaWAN Range Test - RFM96 868MHz

## Overview
This project implements a LoRaWAN range testing node that sends periodic uplinks to ChirpStack containing:
- Packet counter (for packet loss detection)
- RSSI (signal strength at device)
- SNR (signal-to-noise ratio)
- Spreading Factor
- Frequency
- Battery voltage

## Hardware
- **MCU**: Wemos D1 Mini (ESP8266)
- **Radio**: RFM95-868M (SX1276)
- **Region**: EU868

### Pin Connections
- **NSS**: D8 (GPIO15)
- **MOSI**: D7 (GPIO13)
- **MISO**: D6 (GPIO12)
- **SCK**: D5 (GPIO14)
- **DIO0**: D1 (GPIO5)
- **DIO1**: D2 (GPIO4)
- **RESET**: Not connected

## Configuration

### 1. LoRaWAN Credentials
Update in `src/main.cpp` lines 16-18:
```cpp
static const u1_t PROGMEM APPEUI[8] = { ... };  // LSB format (reversed)
static const u1_t PROGMEM DEVEUI[8] = { ... };  // LSB format (reversed)
static const u1_t PROGMEM APPKEY[16] = { ... }; // MSB format (as shown)
```

**Important**: APPEUI and DEVEUI must be reversed from what ChirpStack shows!

### 2. Optional WiFi Logging
To enable HTTP logging to a server:
1. Uncomment `#define USE_WIFI` in `src/main.cpp` line 8
2. Update WiFi credentials lines 9-11
3. Update server URL

### 3. Transmission Interval
Default: 30 seconds (line 41). Adjust as needed for your range test.

## ChirpStack Setup

### 1. Create Device
- Device profile: OTAA, EU868
- Add device with your DEVEUI, APPEUI, and APPKEY

### 2. Add Decoder
Copy the contents of `chirpstack_decoder.js` into:
- Device Profile > Codec > Decoder function

This will decode the binary payload into human-readable values.

## Payload Format
Binary payload (11 bytes):
```
[0-1]   Packet Counter (uint16)
[2]     RSSI + 200 (uint8)
[3]     SNR + 128 (uint8)
[4]     Spreading Factor (uint8)
[5-8]   Frequency in Hz (uint32)
[9-10]  Battery in mV (uint16)
```

## Usage

### 1. Build and Upload
```bash
platformio run --target upload
platformio device monitor
```

### 2. Monitor Serial Output
You'll see:
- Join status
- Range test data for each packet
- TX/RX events

### 3. View in ChirpStack
- Go to your device in ChirpStack
- Check "LoRaWAN frames" tab for uplinks
- View decoded data in "Events" tab

## Range Testing Tips

1. **Start close to gateway** - Verify join and basic communication
2. **Move incrementally** - Walk away gradually, noting RSSI/SNR
3. **Log data** - Use ChirpStack or enable WiFi logging
4. **Check packet loss** - Monitor packet counter for gaps
5. **Note environment** - Indoor vs outdoor, obstacles, etc.

## Troubleshooting

### Not Joining
1. Verify credentials are correct and properly reversed
2. Check gateway is online and in range
3. Verify EU868 region in both device and gateway
4. Check pin connections

### No Uplinks After Join
1. Check serial monitor for errors
2. Verify spreading factor is appropriate for distance
3. Check ChirpStack for downlinks/errors

### Poor Range
1. Check antenna connection
2. Verify 868MHz antenna (not 915MHz)
3. Try higher spreading factor
4. Check for interference

## Library Dependencies
- MCCI LoRaWAN LMIC library v5.0.1

## License
MIT
