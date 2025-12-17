// ChirpStack Payload Decoder for Range Test
// Add this in ChirpStack Device Profile > Codec > Decoder function

function decodeUplink(input) {
    var bytes = input.bytes;
    var decoded = {};

    if (bytes.length >= 11) {
        // Parse binary payload
        // Format: [counter:2][rssi:1][snr:1][sf:1][freq:4][battery:2]
        decoded.packet_counter = (bytes[0] << 8) | bytes[1];
        decoded.rssi = bytes[2] - 200; // Remove offset
        decoded.snr = bytes[3] - 128; // Remove offset
        decoded.spreading_factor = bytes[4];
        decoded.frequency = ((bytes[5] << 24) | (bytes[6] << 16) | (bytes[7] << 8) | bytes[8]) / 1000000.0;
        decoded.battery_voltage = ((bytes[9] << 8) | bytes[10]) / 1000.0;
    }

    return {
        data: decoded
    };
}
