/*
 * EDU-CIAA -> Heltec (por UART software GPIO1 con timer preciso)
 * 
 * Lee sensores y envía datos al Heltec por UART software en GPIO1
 */

#include "sapi.h"
#include "leer_adc.h"
#include "sapi_dht11.h"

#define TX_PIN  GPIO1  // Pin 32 - TX hacia Heltec
#define DHT11_PIN GPIO7 // Pin para el sensor DHT11

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
    
    // Inicializar sensor DHT11
    dht11Init(DHT11_PIN);
      
    // Configurar GPIO1 como salida (TX)
    gpioInit(TX_PIN, GPIO_OUTPUT);
    gpioWrite(TX_PIN, HIGH); // Idle state
    
    printf("\n=== EDU-CIAA -> HELTEC (GPIO1 software UART) ===\r\n");
    printf("Sensores: Luz(CH3), HumSuelo(CH2), DHT11(GPIO0)\r\n");
    printf("Enviando datos cada 30 segundos...\r\n\n");
    
    int contador = 0;
    
    while(1) {
        // Leer sensores analógicos (UNA SOLA VEZ cada uno)
        uint8_t luz_percent = leerADC(CH3);
        delay(500);
        uint8_t hum_suelo = leerADC(CH2);
        
        // Variables para DHT11
        float temperatura = 0;
        float humedad_aire = 0;
        
        // Leer DHT11 (temperatura y humedad del aire)
        bool dht_ok = dht11Read(&humedad_aire, &temperatura);
        
        // Convertir a enteros
        int temp_int = (int)temperatura;
        int hum_int = (int)humedad_aire;
        
        if (!dht_ok) {
            printf("Error leyendo DHT11 en GPIO7\r\n");
        }
        
        // Crear mensaje con todos los sensores
        char mensaje[100];
        sprintf(mensaje, "temp:%d,hum_air:%d,luz:%d,hum_soil:%d,count:%d\n", 
                temp_int, hum_int, luz_percent, hum_suelo, contador);
        
        // Debug en USB
        printf("[TX %d] %s", contador, mensaje);
        
        // Enviar por GPIO1 (software UART) al Heltec
        softUART_writeString(mensaje);
        
        contador++;
        delay(5000);  // 7 segundos
    }
    
    return 0;
}
