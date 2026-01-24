/*
 * EDU-CIAA -> Heltec (por UART)
 * 
 * Lee sensores y envía datos al Heltec por UART software en GPIO1
 */

#include "sapi.h"
#include "leer_adc.h"

#define TX_PIN  GPIO1  // Pin 32 - TX hacia Heltec

// Enviar un byte por UART software a 9600 baud
void softUART_writeByte(uint8_t data) {
    uint32_t bitDelay = 104; // microsegundos por bit a 9600 baud
    
    // START bit (LOW)
    gpioWrite(TX_PIN, LOW);
    delayInaccurateUs(bitDelay);
    
    // 8 bits de datos (LSB primero)
    for(int i = 0; i < 8; i++) {
        if(data & (1 << i)) {
            gpioWrite(TX_PIN, HIGH);
        } else {
            gpioWrite(TX_PIN, LOW);
        }
        delayInaccurateUs(bitDelay);
    }
    
    // STOP bit (HIGH)
    gpioWrite(TX_PIN, HIGH);
    delayInaccurateUs(bitDelay);
}

void softUART_writeString(const char* str) {
    while(*str) {
        softUART_writeByte(*str);
        str++;
    }
}

int main(void) {
    boardInit();
    
    // UART_USB para debug
    uartConfig(UART_USB, 115200);
    
    // Configurar GPIO1 como salida (TX)
    gpioInit(TX_PIN, GPIO_OUTPUT);
    gpioWrite(TX_PIN, HIGH); // Idle state
    
    printf("\n=== EDU-CIAA -> HELTEC (GPIO1 software UART) ===\r\n");
    printf("Enviando datos cada 5 segundos...\r\n\n");
    
    int contador = 0;
    
    while(1) {
        // Leer sensor de luz
        uint16_t luz_raw = leerADC(CH1);
        int luz_percent = (luz_raw * 100) / 1023;
        
        // Crear mensaje
        char mensaje[100];
        sprintf(mensaje, "temp:24.5,hum_air:58,luz:%d,hum_soil:42,count:%d\n", 
                luz_percent, contador);
        
        // Enviar por GPIO1 (software UART) al Heltec
        softUART_writeString(mensaje);
        
        // Debug en USB
        printf("[TX %d] %s", contador, mensaje);
        
        contador++;
        delay(5000);
    }
    
    return 0;
}
