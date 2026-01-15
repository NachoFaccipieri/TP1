/*
 * EDU-CIAA Transmisor LoRa
 * Lee sensor de luz y transmite por LoRa usando Wio-SX1262
 * (Versión simplificada - solo sensor de luz + datos ficticios)
 */

#include "sapi.h"
#include "leer_adc.h"
#include "sx1262_lora.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    // Variables
    uint8_t porcentajeLuz;
    char mensaje[100];
    uint8_t contador = 0;
    
    // ----- INICIALIZACIÓN -----
    boardConfig();
    uartConfig(UART_USB, 115200);
    adcConfig(ADC_ENABLE);
    
    printf("\n=== EDU-CIAA TRANSMISOR LORA (MODO PRUEBA) ===\r\n");
    printf("Inicializando SX1262...\r\n");
    
    // Inicializar LoRa
    sx1262_init();
    
    printf("SX1262 inicializado en 915 MHz\r\n");
    printf("Transmitiendo cada 3 segundos...\r\n\n");
    
    // ----- LOOP PRINCIPAL -----
    while(true) {
        
        // Leer SOLO el sensor de luz (el que funciona)
        porcentajeLuz = leerADC(3);
        
        // Crear mensaje con luz REAL y datos ficticios para los demás
        // Formato: temp:25.5,hum_air:60,luz:XX,hum_soil:45,count:N
        sprintf(mensaje, "temp:24.5,hum_air:58,luz:%d,hum_soil:42,count:%d", 
                porcentajeLuz, contador);
        
        // Mostrar en consola
        printf("TX [%d]: %s\r\n", contador, mensaje);
        
        // Transmitir por LoRa
        sx1262_transmit(mensaje, strlen(mensaje));
        
        printf("  -> Enviado OK\r\n\n");
        
        contador++;
        
        // Esperar 3 segundos
        delay(3000);
    }
    
    return 0;
}
