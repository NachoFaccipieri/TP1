#include "spi_bitbang.h"
#include "sapi.h"

// Pines según tu PCB
#define BB_NSS_PIN   GPIO5
#define BB_SCK_PIN   GPIO3  
#define BB_MOSI_PIN  ENET_TXD1  // Pin 21 P2 - SPI_MOSI (P1_2 del LPC4337)
#define BB_MISO_PIN  ENET_MDIO  // Pin 18 P2 - SPI_MISO (intentando)

void spi_bitbang_init(void) {
    gpioInit(BB_NSS_PIN, GPIO_OUTPUT);
    gpioInit(BB_SCK_PIN, GPIO_OUTPUT);
    gpioInit(BB_MOSI_PIN, GPIO_OUTPUT);
    gpioInit(BB_MISO_PIN, GPIO_INPUT);
    
    gpioWrite(BB_SCK_PIN, LOW);
    gpioWrite(BB_NSS_PIN, HIGH);
}

void spi_bitbang_transfer(const uint8_t* tx_buffer, uint8_t* rx_buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        uint8_t tx_byte = tx_buffer ? tx_buffer[i] : 0x00;
        uint8_t rx_byte = 0;
        
        for (int bit = 7; bit >= 0; bit--) {
            // Escribir bit en MOSI
            gpioWrite(BB_MOSI_PIN, (tx_byte >> bit) & 0x01);
            
            // Clock alto
            gpioWrite(BB_SCK_PIN, HIGH);
            delayInaccurateUs(5);
            
            // Leer bit de MISO
            if (gpioRead(BB_MISO_PIN)) {
                rx_byte |= (1 << bit);
            }
            
            // Clock bajo
            gpioWrite(BB_SCK_PIN, LOW);
            delayInaccurateUs(5);
        }
        
        if (rx_buffer) {
            rx_buffer[i] = rx_byte;
        }
    }
}


