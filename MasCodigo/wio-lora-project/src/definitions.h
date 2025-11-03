#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// LoRa configuration constants
#define LORA_FREQUENCY 915E6  // Frequency in Hz
#define LORA_SPREADING_FACTOR 7 // Spreading factor (7 to 12)
#define LORA_BANDWIDTH 125E3    // Bandwidth in Hz
#define LORA_CODING_RATE 5      // Coding rate (5 to 8)
#define LORA_SYNC_WORD 0x34    // Sync word
#define LORA_TX_POWER 20       // Transmit power

// Maximum payload size
#define MAX_PAYLOAD_SIZE 255

// Gateway definitions
#define GATEWAY_ADDRESS "your_gateway_address"
#define GATEWAY_PORT 1234

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