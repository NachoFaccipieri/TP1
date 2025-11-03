/*=============================================================================
 * Ejemplo de Transmisor LoRa con SX1262 y EDU-CIAA (SAPI)
 * VERSIÓN CORREGIDA
 *===========================================================================*/

#include "sapi.h"
#include "sx126x.h"
#include "sx126x_hal.h" // Incluimos el HAL para tener sx126x_wait_on_busy

// --- Configuración de LoRa ---
#define RF_FREQUENCY          915000000 // Frecuencia en Hz (915 MHz)
#define TX_OUTPUT_POWER       14        // Potencia de TX en dBm
// --- CORRECCIÓN: Usar las constantes del driver ---
#define LORA_BANDWIDTH        SX126X_LORA_BW_125
#define LORA_SPREADING_FACTOR SX126X_LORA_SF7
#define LORA_CODINGRATE       SX126X_LORA_CR_4_5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_SYMBOL_TIMEOUT   0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON  false

int main( void )
{
    // Inicialización de la placa y periféricos
    boardConfig();
    spiConfig( SPI0 );
    
    gpioConfig(GPIO0, GPIO_OUTPUT);
    gpioConfig(GPIO1, GPIO_INPUT);
    gpioConfig(GPIO2, GPIO_OUTPUT);
    gpioConfig(GPIO3, GPIO_INPUT);
    
    // --- CORRECCIÓN: Usar 0/1 en lugar de GPIO_LEVEL_... ---
    gpioWrite(GPIO0, 1); // NSS en alto por defecto

    uartConfig( UART_USB, 115200 );
    printf("Inicializando transceptor LoRa SX1262...\r\n");

    // --- Inicialización del SX1262 ---
    // --- CORRECCIÓN: Todas las funciones ahora reciben NULL como primer argumento ---
    sx126x_reset( NULL );
    
    sx126x_set_reg_mode( NULL, SX126X_REG_MODE_DCDC );
    
    sx126x_set_pkt_type( NULL, SX126X_PKT_TYPE_LORA );
    
    sx126x_set_rf_freq( NULL, RF_FREQUENCY );
    
    // --- CORRECCIÓN: Configurar modulación usando una estructura ---
    sx126x_mod_params_lora_t mod_params;
    mod_params.sf   = LORA_SPREADING_FACTOR;
    mod_params.bw   = LORA_BANDWIDTH;
    mod_params.cr   = LORA_CODINGRATE;
    mod_params.ldro = 0; // Low Data Rate Optimize: 0 para SF7/BW125
    sx126x_set_lora_mod_params( NULL, &mod_params );

    // --- CORRECCIÓN: Configurar paquete usando una estructura ---
    sx126x_pkt_params_lora_t pkt_params;
    pkt_params.preamble_len_in_symb = LORA_PREAMBLE_LENGTH;
    pkt_params.header_type          = SX126X_LORA_PKT_EXPLICIT;
    pkt_params.pld_len_in_bytes     = 255;
    pkt_params.crc_is_on            = true;
    pkt_params.invert_iq_is_on      = false;
    sx126x_set_lora_pkt_params( NULL, &pkt_params );
    
    printf("SX1262 configurado como transmisor.\r\n");

    uint8_t buffer[] = "Hola EDU-CIAA!";
    uint32_t counter = 0;

    while( true )
    {
        printf("Enviando paquete %d: %s\r\n", counter, buffer);

        // Configurar potencia de salida
        sx126x_set_tx_params( NULL, TX_OUTPUT_POWER, SX126X_RAMP_200_US );

        // --- CORRECCIÓN: El envío se hace en dos pasos ---
        // 1. Escribir el payload al buffer interno del chip
        sx126x_write_buffer( NULL, 0, buffer, sizeof(buffer) );
        // 2. Poner el chip en modo TX para que envíe lo que está en el buffer
        sx126x_set_tx( NULL, 2000 ); // Timeout de 2 segundos

        // Esperar hasta que la transmisión termine (monitoreando el pin BUSY)
        // Esta función la creamos nosotros en sx126x-hal.c
        sx126x_hal_wait_on_busy();

        printf("Paquete enviado.\r\n\n");
        
        counter++;
        delay(5000);
    }

    return 0;
}