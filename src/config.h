// config.h - Configuration file for Range Test
// Edit this file to configure WiFi and server settings

#ifndef CONFIG_H
#define CONFIG_H

// =========================================
// WiFi Configuration
// =========================================
// Uncomment the line below to enable WiFi logging
#define USE_WIFI

#ifdef USE_WIFI
    // Your WiFi credentials
    #define WIFI_SSID "guest"
    #define WIFI_PASSWORD "grundfosguest08"

    // Server configuration
    // Replace YOUR_COMPUTER_IP with the IP address of the computer running server.py
    // Example: "http://192.168.1.100:9000/api/rangetest"
    #define SERVER_URL "http://172.16.0.82:9000/api/rangetest"
#endif

// =========================================
// LoRaWAN Configuration
// =========================================
// Region is configured in: .pio/libdeps/d1_mini/MCCI LoRaWAN LMIC library/project_config/lmic_project_config.h
// Current: EU868

// Transmission interval (seconds)
#define TX_INTERVAL 30  // Send packet every 30 seconds

// =========================================
// LoRaWAN Credentials (ChirpStack OTAA)
// =========================================
// IMPORTANT: APPEUI and DEVEUI must be in LSB format (reversed from ChirpStack UI)
//            APPKEY must be in MSB format (same as ChirpStack UI)

// Join EUI (Application EUI) - LSB format (reversed)
// ChirpStack shows: 63d51c014a33cc7e
// Enter as:         7E CC 33 4A 01 1C D5 63 (reversed)
static const uint8_t PROGMEM APPEUI[8] = { 0x7E, 0xCC, 0x33, 0x4A, 0x01, 0x1C, 0xD5, 0x63 };

// Device EUI - LSB format (reversed)
// ChirpStack shows: 225c47df264f266a
// Enter as:         6A 26 4F 26 DF 47 5C 22 (reversed)
static const uint8_t PROGMEM DEVEUI[8] = { 0x6A, 0x26, 0x4F, 0x26, 0xDF, 0x47, 0x5C, 0x22 };

// Application Key - MSB format (same as ChirpStack)
// ChirpStack shows: bcd0caf24cdd6dfe633a8040506c4e2d
// Enter as:         BC D0 CA F2 4C DD 6D FE 63 3A 80 40 50 6C 4E 2D (same)
static const uint8_t PROGMEM APPKEY[16] = { 0xBC, 0xD0, 0xCA, 0xF2, 0x4C, 0xDD, 0x6D, 0xFE, 0x63, 0x3A, 0x80, 0x40, 0x50, 0x6C, 0x4E, 0x2D };

#endif // CONFIG_H
