/*
 * SX1262 LoRa Driver para EDU-CIAA
 * Implementación simplificada para transmisión
 */

#include "sx1262_lora.h"

// Variables privadas
static bool inicializado = false;

// Esperar a que BUSY esté bajo
static void sx1262_wait_busy(void) {
    uint32_t timeout = 1000000;
    uint32_t count = 0;
    while (gpioRead(SX1262_BUSY) && timeout > 0) {
        timeout--;
        count++;
    }
    if (timeout == 0) {
        printf("  [ERROR] TIMEOUT esperando BUSY!\r\n");
    }
    if (count > 0) {
        printf("  [DEBUG] BUSY esperó %lu ciclos\r\n", count);
    }
}

// Enviar comando SPI
static void sx1262_write_command(uint8_t cmd, uint8_t* data, uint8_t length) {
    sx1262_wait_busy();
    
    gpioWrite(SX1262_NSS, LOW);
    delay(1);
    
    spiWrite(SPI0, &cmd, 1);
    if (length > 0 && data != NULL) {
        spiWrite(SPI0, data, length);
    }
    
    delay(1);
    gpioWrite(SX1262_NSS, HIGH);
    
    sx1262_wait_busy();
}

// Inicializar SX1262
void sx1262_init(void) {
    uint8_t buffer[8];
    
    printf("  [DEBUG] Configurando pines...\r\n");
    
    // Configurar pines
    gpioConfig(SX1262_NSS, GPIO_OUTPUT);
    gpioConfig(SX1262_RST, GPIO_OUTPUT);
    gpioConfig(SX1262_BUSY, GPIO_INPUT);
    
    gpioWrite(SX1262_NSS, HIGH);
    
    printf("  [DEBUG] NSS=%d, RST=%d, BUSY=%d (pines GPIO)\r\n", 
           SX1262_NSS, SX1262_RST, SX1262_BUSY);
    
    // Reset del módulo
    printf("  [DEBUG] Haciendo reset del SX1262...\r\n");
    gpioWrite(SX1262_RST, LOW);
    delay(20);
    gpioWrite(SX1262_RST, HIGH);
    delay(50);
    
    printf("  [DEBUG] Esperando BUSY...\r\n");
    sx1262_wait_busy();
    printf("  [DEBUG] BUSY OK (chip respondió)\r\n");
    
    // Configurar SPI
    printf("  [DEBUG] Configurando SPI...\r\n");
    spiConfig(SPI0);
    
    // *** CONFIGURAR REGULADOR (CRÍTICO) ***
    printf("  [DEBUG] Configurando regulador (DC-DC)...\r\n");
    buffer[0] = 0x00; // Usar DC-DC
    sx1262_write_command(0x96, buffer, 1); // SetRegulatorMode
    
    // Poner en STANDBY
    printf("  [DEBUG] Enviando comando STANDBY...\r\n");
    sx1262_write_command(SX1262_CMD_SET_STANDBY, NULL, 0);
    delay(10);
    
    // *** CONFIGURAR TCXO (CRÍTICO - SIN ESTO NO TRANSMITE) ***
    printf("  [DEBUG] Configurando TCXO 1.7V...\r\n");
    buffer[0] = 0x01; // 1.7V
    buffer[1] = 0x00; // Timeout MSB
    buffer[2] = 0x00; // Timeout
    buffer[3] = 0x40; // Timeout LSB (64ms)
    sx1262_write_command(0x97, buffer, 4); // SetDIO3AsTCXOCtrl
    delay(10);
    
    // *** CALIBRAR (OBLIGATORIO) ***
    printf("  [DEBUG] Calibrando chip...\r\n");
    buffer[0] = 0x7F; // Calibrar todo (RC64k, RC13M, PLL, ADC, Image)
    sx1262_write_command(0x89, buffer, 1); // Calibrate
    delay(50); // Esperar calibración
    
    // Calibrar imagen para 868 MHz
    printf("  [DEBUG] Calibrando imagen para 868 MHz...\r\n");
    buffer[0] = 0xE1; // freq1 (863-870 MHz)
    buffer[1] = 0xE9; // freq2
    sx1262_write_command(0x9B, buffer, 2); // CalibrateImage
    delay(10);
    
    // Configurar tipo de paquete (LoRa)
    printf("  [DEBUG] Configurando modo LoRa...\r\n");
    buffer[0] = 0x01; // LoRa
    sx1262_write_command(0x8A, buffer, 1); // SetPacketType
    
    // Configurar DIO2 como RF switch
    printf("  [DEBUG] Configurando DIO2...\r\n");
    buffer[0] = 0x01;
    sx1262_write_command(SX1262_CMD_SET_DIO2_AS_RF_SW, buffer, 1);
    
    // Configurar frecuencia 868.3 MHz (test)
    printf("  [DEBUG] Configurando frecuencia 868.3 MHz...\r\n");
    // freq = (frf * 32000000) / 2^25
    // frf = (868300000 * 2^25) / 32000000 = 910557184 = 0x3644CCCC (approx 0x3644C000)
    buffer[0] = 0x36;
    buffer[1] = 0x44;
    buffer[2] = 0xCC;
    buffer[3] = 0xCC;
    sx1262_write_command(SX1262_CMD_SET_RF_FREQUENCY, buffer, 4);
    
    // Configurar modulación LoRa: SF12, BW125, CR4/5 (IGUAL AL EJEMPLO)
    printf("  [DEBUG] Configurando modulación (SF12, BW125, CR4/5)...\r\n");
    buffer[0] = 0x0C;  // SF12 (no SF7)
    buffer[1] = 0x04;  // BW125
    buffer[2] = 0x01;  // CR 4/5
    buffer[3] = 0x01;  // LowDataRateOptimize on
    sx1262_write_command(SX1262_CMD_SET_MODULATION, buffer, 4);
    
    // Configurar PA (Power Amplifier) - potencia media
    printf("  [DEBUG] Configurando PA (17 dBm)...\r\n");
    buffer[0] = 0x04;  // PA duty cycle
    buffer[1] = 0x07;  // HP max
    buffer[2] = 0x00;  // Device sel
    buffer[3] = 0x01;  // PA LUT
    sx1262_write_command(SX1262_CMD_SET_PA_CONFIG, buffer, 4);
    
    // Configurar potencia de transmisión
    buffer[0] = 0x0E; // 22 dBm
    buffer[1] = 0x04; // Ramping time
    sx1262_write_command(0x8E, buffer, 2); // SetTxParams
    
    // Configurar Buffer Base Address
    buffer[0] = 0x00; // TX base
    buffer[1] = 0x00; // RX base
    sx1262_write_command(0x8F, buffer, 2); // SetBufferBaseAddress
    
    // Configurar Sync Word (0x21 = como ejemplo)
    buffer[0] = 0x14;
    buffer[1] = 0x24;
    sx1262_write_command(0x9F, buffer, 2); // SetLoRaSyncWord
    
    printf("  [DEBUG] SX1262 configurado OK!\r\n");
    
    inicializado = true;
}

// Transmitir datos
void sx1262_transmit(const char* data, uint8_t length) {
    if (!inicializado) {
        printf("  [ERROR] SX1262 no inicializado!\r\n");
        return;
    }
    
    uint8_t buffer[256];
    
    printf("  [TX] Iniciando transmisión de %d bytes...\r\n", length);
    
    // 1. Poner en STANDBY
    buffer[0] = 0x00; // STDBY_RC
    sx1262_write_command(0x80, buffer, 1); // SetStandby
    delay(5);
    printf("  [TX] Modo STANDBY OK\r\n");
    
    // 2. Escribir datos al buffer TX
    sx1262_wait_busy();
    gpioWrite(SX1262_NSS, LOW);
    delay(1);
    
    uint8_t cmd = 0x0E; // WriteBuffer
    spiWrite(SPI0, &cmd, 1);
    uint8_t offset = 0x00;
    spiWrite(SPI0, &offset, 1);
    spiWrite(SPI0, (uint8_t*)data, length);
    
    delay(1);
    gpioWrite(SX1262_NSS, HIGH);
    sx1262_wait_busy();
    printf("  [TX] Buffer escrito OK\r\n");
    
    // 3. Configurar parámetros del paquete
    buffer[0] = 0x00;   // PreambleLengthMSB
    buffer[1] = 0x08;   // PreambleLengthLSB (8 symbols)
    buffer[2] = 0x00;   // HeaderType (Variable length)
    buffer[3] = length; // PayloadLength
    buffer[4] = 0x01;   // CRC ON
    buffer[5] = 0x00;   // InvertIQ standard
    sx1262_write_command(0x8C, buffer, 6); // SetPacketParams
    printf("  [TX] Parámetros del paquete OK\r\n");
    
    // 4. Limpiar IRQ
    buffer[0] = 0x03; // Clear all
    buffer[1] = 0xFF;
    sx1262_write_command(0x02, buffer, 2); // ClearIrqStatus
    
    // 5. Configurar DIO1 para TxDone
    buffer[0] = 0x00;
    buffer[1] = 0x01; // TxDone en DIO1
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    sx1262_write_command(0x08, buffer, 8); // SetDioIrqParams
    
    // 6. Transmitir con timeout
    printf("  [TX] Enviando comando SetTx...\r\n");
    buffer[0] = 0x00; // Timeout 15.625ms * 64 = 1 segundo
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    sx1262_write_command(0x83, buffer, 3); // SetTx
    
    printf("  [TX] Comando TX enviado, esperando...\r\n");
    // Esperar TX
    delay(2000);
    printf("  [TX] Transmisión completada\r\n");
}
