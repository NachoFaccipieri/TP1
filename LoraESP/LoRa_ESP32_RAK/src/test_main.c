/*
 * EDU-CIAA -> Heltec (por UART software GPIO1 con timer preciso)
 * 
 * Lee sensores y envía datos al Heltec por UART software en GPIO1
 */

#include "sapi.h"
#include "leer_adc.h"

#define TX_PIN  GPIO1  // Pin 32 - TX hacia Heltec

// Delay preciso usando delay() para 300 baud
void preciseBitDelay(void) {
    // 300 baud = 3.33 ms por bit (ultra lento pero estable)
    delay(4);  // 3 milisegundos, muy estable
}

// Enviar un byte por UART software
void softUART_writeByte(uint8_t data) {
    // START bit (LOW)
    gpioWrite(TX_PIN, LOW);
    preciseBitDelay();
    
    // 8 bits de datos (LSB primero)
    for(int i = 0; i < 8; i++) {
        if(data & (1 << i)) {
            gpioWrite(TX_PIN, HIGH);
        } else {
            gpioWrite(TX_PIN, LOW);
        }
        preciseBitDelay();
    }
    
    // STOP bit (HIGH)
    gpioWrite(TX_PIN, HIGH);
    preciseBitDelay();
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
    
    adcConfig(ADC_ENABLE);
      
    // Configurar GPIO1 como salida (TX)
    gpioInit(TX_PIN, GPIO_OUTPUT);
    gpioWrite(TX_PIN, HIGH); // Idle state
    
    printf("\n=== EDU-CIAA -> HELTEC (GPIO1 software UART optimizado) ===\r\n");
    printf("Enviando datos cada 5 segundos...\r\n\n");
    
    int contador = 0;
    
    while(1) {
        // Leer sensor de luz (canal 3 según código viejo)
        uint16_t luz_raw = adcRead(CH3);  // Cambiado a CH3
        uint8_t luz_percent = leerADC(CH3);  // Cambiado a CH3
        
        // Debug en USB - mostrar valor RAW
        printf("ADC RAW: %d (de 1023)\r\n", luz_raw);
        
        // Crear mensaje
        char mensaje[100];
        sprintf(mensaje, "temp:24.5,hum_air:58,luz:%d,hum_soil:42,count:%d\n", 
                luz_percent, contador);
        
        // Enviar por GPIO1 (software UART) al Heltec
        softUART_writeString(mensaje);
        
        // Debug en USB
        printf("[TX %d] %s", contador, mensaje);
        
        contador++;
        delay(30000);  // 30 segundos en vez de 5
    }
    
    return 0;
}
