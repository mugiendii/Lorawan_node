# LoRaWAN Range Test Node - SX1276/RFM95 868MHz

LoRaWAN range testing node (ESP8266 + RFM95-868M/SX1276) that sends periodic
uplinks to ChirpStack containing packet counter, RSSI, SNR, spreading factor,
frequency, and battery voltage.

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

## Project Structure

```
src/
├── main.cpp      # Serial commands, callbacks, result display
└── config.h      # WiFi/LoRaWAN credentials, TX interval, feature flags

lib/
├── LoRaWAN/       # LoRaWAN protocol library (LMIC integration, OTAA join,
│                  # uplink transmission, range test data) - see its README
│                  # for the full API
└── communication/ # WiFi/HTTP logging
```

## Configuration

### 1. LoRaWAN Credentials
Update in `src/config.h`:
```cpp
static const u1_t PROGMEM APPEUI[8] = { ... };  // LSB format (reversed)
static const u1_t PROGMEM DEVEUI[8] = { ... };  // LSB format (reversed)
static const u1_t PROGMEM APPKEY[16] = { ... }; // MSB format (as shown)
```

**Important**: APPEUI and DEVEUI must be reversed from what ChirpStack shows!

### 2. Optional WiFi Logging
To enable HTTP logging to a server:
1. Uncomment `#define USE_WIFI` in `src/config.h`
2. Update WiFi credentials
3. Update server URL

### 3. Transmission Interval
Default: 30 seconds (`TX_INTERVAL`). Adjust as needed for your range test.

## ChirpStack Setup

1. Create a device with profile OTAA, EU868, using your DEVEUI/APPEUI/APPKEY.
2. Add the decoder: copy `chirpstack_decoder.js` into Device Profile > Codec >
   Decoder function to decode the binary payload into readable values.

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

```bash
platformio run --target upload
platformio device monitor
```

The device boots in AUTO mode (auto-join + periodic uplinks). For manual
control (JOIN/TRIGGER/STATUS/etc.), see [SERIAL_COMMANDS.md](SERIAL_COMMANDS.md).

## Range Testing Tips

1. **Start close to gateway** - verify join and basic communication
2. **Move incrementally** - walk away gradually, noting RSSI/SNR
3. **Check packet loss** - monitor packet counter for gaps
4. **Note environment** - indoor vs outdoor, obstacles, etc.

## Troubleshooting

### Not Joining
- Verify credentials are correct and properly reversed
- Check gateway is online and in range
- Verify EU868 region in both device and gateway
- Check pin connections

### No Uplinks After Join
- Check serial monitor for errors
- Verify spreading factor is appropriate for distance
- Check ChirpStack for downlinks/errors

### Poor Range
- Check antenna connection (868MHz, not 915MHz)
- Try higher spreading factor
- Check for interference

## Library Dependencies
- MCCI LoRaWAN LMIC library v5.0.1

## License
MIT
