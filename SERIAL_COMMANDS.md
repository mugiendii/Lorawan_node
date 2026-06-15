# Serial Commands Guide

## Overview
The LoRaWAN Range Test Node now supports serial commands for manual control of the device. This allows you to manually trigger join procedures and uplink transmissions for range testing.

## Operating Modes

### AUTO Mode (Default)
- Device automatically joins the network on startup
- Sends periodic uplinks every 30 seconds (configurable via TX_INTERVAL)
- Continuous range testing

### MANUAL Mode
- Device waits for user commands
- JOIN and TRIGGER commands must be issued manually
- Ideal for controlled testing scenarios

## Available Commands

### JOIN
**Description:** Initiates OTAA (Over-The-Air Activation) join procedure with the LoRaWAN network.

**Usage:**
```
JOIN
```

**Response:**
- Success: Displays session keys and "JOIN: SUCCESS" message
- Failure: "JOIN: FAILED" with error details

**Notes:**
- Only works if not already joined
- Returns error if already joined to network

---

### TRIGGER
**Description:** Sends an uplink packet and returns detailed range test results.

**Usage:**
```
TRIGGER
```

**Response - Success:**
```
======================
RANGE TEST: SUCCESS
======================
Packet #: 1

--- Uplink Quality ---
RSSI: -45 dBm
SNR: 9 dB
SF: 7
Freq: 868.1 MHz

--- Downlink Quality ---
Gateway RSSI: -43 dBm
Gateway SNR: 10 dB

--- Estimated Range ---
Distance: ~0.10 km
Link Quality: EXCELLENT
======================
```

**Response - Failure:**
```
======================
RANGE TEST: FAILED
No response received
Status: OUT OF RANGE
======================
```

**Notes:**
- Device must be joined to network first (use JOIN command)
- Sends confirmed uplink (requests ACK from gateway)
- Returns error if radio is busy or not joined

---

### AUTO
**Description:** Enables automatic transmission mode.

**Usage:**
```
AUTO
```

**Effect:**
- Switches to continuous automatic transmission
- Device sends uplinks every TX_INTERVAL seconds
- Automatically rejoins if connection lost

---

### MANUAL
**Description:** Enables manual command mode.

**Usage:**
```
MANUAL
```

**Effect:**
- Stops automatic transmissions
- Requires explicit JOIN and TRIGGER commands
- Gives full manual control over device behavior

---

### STATUS
**Description:** Displays current device status.

**Usage:**
```
STATUS
```

**Response Example:**
```
Mode: MANUAL
Joined: YES
DevAddr: 260B1234
Packets sent: 5
```

---

### HELP
**Description:** Shows list of available commands.

**Usage:**
```
HELP
```

---

## Typical Usage Scenarios

### Scenario 1: Manual Range Testing
1. Start device (boots in AUTO mode)
2. Send `MANUAL` command to switch to manual mode
3. Send `JOIN` command to join network
4. Walk to test location
5. Send `TRIGGER` command to test range
6. Review range test results
7. Move to new location and repeat step 5

### Scenario 2: Quick Auto Test
1. Start device (auto joins and starts transmitting)
2. Walk around with device
3. Monitor serial output for continuous range data
4. Send `STATUS` to check statistics anytime

### Scenario 3: Controlled Testing
1. Start device
2. Send `MANUAL` command immediately
3. Send `JOIN` command to connect
4. Set up test equipment at specific locations
5. Send `TRIGGER` at each location for precise measurements
6. Compare results across locations

## Range Test Result Interpretation

### Link Quality Ratings
- **EXCELLENT** (RSSI > -70 dBm): Very close range, strong signal
- **GOOD** (RSSI > -90 dBm): Medium range, good signal quality
- **FAIR** (RSSI > -110 dBm): Long range, acceptable signal
- **POOR** (RSSI ≤ -110 dBm): Maximum range, weak signal

### Distance Estimates
The distance estimates are approximate and based on RSSI:
- < 50m: RSSI > -50 dBm
- 50-200m: RSSI -50 to -70 dBm
- 200-500m: RSSI -70 to -90 dBm
- 500m-2km: RSSI -90 to -110 dBm
- > 2km: RSSI < -110 dBm

**Note:** Actual range depends heavily on:
- Environment (urban, rural, indoor, outdoor)
- Obstacles (buildings, trees, terrain)
- Antenna quality and placement
- Gateway height and sensitivity
- Spreading factor and transmission power

## Serial Terminal Settings
- Baud Rate: 115200
- Line Ending: Newline (\n) or CR+LF
- Commands are case-insensitive

## Troubleshooting

**Problem:** JOIN command fails
- **Solution:** Check LoRaWAN credentials in config.h
- **Solution:** Verify gateway is online and reachable
- **Solution:** Check antenna connection

**Problem:** TRIGGER shows "OUT OF RANGE"
- **Solution:** Move closer to gateway
- **Solution:** Check gateway is receiving uplinks (check gateway logs)
- **Solution:** Verify spreading factor allows sufficient range

**Problem:** Commands not responding
- **Solution:** Check serial connection (115200 baud)
- **Solution:** Ensure line ending is set correctly
- **Solution:** Type HELP to verify communication

**Problem:** "Radio busy" error
- **Solution:** Wait for current transmission to complete
- **Solution:** Check device is not stuck in a transmission loop
