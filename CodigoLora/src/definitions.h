#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// LoRa configuration constants
// Use integer literal for frequency to avoid float/preprocessor issues
#define LORA_FREQUENCY 915000000UL  // Frequency in Hz (915 MHz)
#define LORA_SPREADING_FACTOR 7     // Spreading factor (7 to 12)
#define LORA_BANDWIDTH 125000       // Bandwidth in Hz
// Coding rate: 5..8 corresponds to 4/5 .. 4/8 (LMIC/SX126x convention)
#define LORA_CODING_RATE 5
#define LORA_SYNC_WORD 0x34         // Sync word
#define LORA_TX_POWER 14            // Transmit power in dBm

// Maximum payload size
#define MAX_PAYLOAD_SIZE 255

// Gateway definitions (optional; LMIC/OTAA doesn't need these directly)
// Adjust PORT if your gateway uses a different packet-forwarder port
#define GATEWAY_ADDRESS "your_gateway_address" // placeholder, optional
#define GATEWAY_PORT 1680                       // common packet-forwarder UDP port

// Other common definitions
typedef enum {
    LORA_OK,
    LORA_ERROR,
    LORA_TIMEOUT
} lora_status_t;

// Serial configuration
#define EDUCIAA_BAUDRATE 115200
#define SERIAL_TIMEOUT 1000
#define BUFFER_SIZE 256

#endif // DEFINITIONS_H