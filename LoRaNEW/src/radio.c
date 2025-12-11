#include "radio.h"
#include "sx126x.h"
#include "sx126x_hal_context.h"
#include "sapi.h"
#include <string.h>

// Contexto del sx126x con los GPIOs correctos
static uint8_t sx126x_context_buffer[100];
static sx126x_hal_context_t sx126x_context = {
    .nss_pin = GPIO5,    // Pin 36
    .busy_pin = GPIO1,   // Pin 32
    .reset_pin = GPIO3,  // Pin 34
    .dio1_pin = 0        // No conectado
};

static RadioEvents_t* RadioEvents = NULL;
static RadioState_t RadioState = RF_IDLE;

// Buffer de recepción
static uint8_t RxBuffer[255];
static uint8_t RxBufferSize = 0;

void Radio_Init(RadioEvents_t* events) {
    RadioEvents = events;
    
    uartWriteString(UART_USB, "Inicializando GPIOs...\r\n");
    
    // Inicializar GPIOs
    gpioInit(GPIO5, GPIO_OUTPUT);  // NSS
    gpioInit(GPIO1, GPIO_INPUT);   // BUSY
    gpioInit(GPIO3, GPIO_OUTPUT);  // RESET
    
    gpioWrite(GPIO5, HIGH);  // NSS alto
    
    uartWriteString(UART_USB, "Haciendo RESET del SX1262...\r\n");
    
    // Reset del SX1262
    gpioWrite(GPIO3, LOW);
    delay(20);
    gpioWrite(GPIO3, HIGH);
    delay(100);
    
    uartWriteString(UART_USB, "Esperando BUSY...\r\n");
    
    // Verificar BUSY
    int busy_wait = 0;
    while (gpioRead(GPIO1) == HIGH && busy_wait < 1000) {
        delay(1);
        busy_wait++;
    }
    
    if (busy_wait >= 1000) {
        uartWriteString(UART_USB, "ERROR: BUSY timeout!\r\n");
    } else {
        uartWriteString(UART_USB, "BUSY OK\r\n");
    }
    
    uartWriteString(UART_USB, "Configurando SX1262...\r\n");
    
    // Configuración básica del SX1262
    sx126x_status_t status = sx126x_set_standby(&sx126x_context, SX126X_STANDBY_CFG_RC);
    
    // Configurar regulador de voltaje
    sx126x_set_reg_mode(&sx126x_context, SX126X_REG_MODE_DCDC);
    
    // Configurar tipo de paquete
    sx126x_set_pkt_type(&sx126x_context, SX126X_PKT_TYPE_LORA);
    
    // Configurar frecuencia inicial (916.8 MHz - AU915 canal 8)
    sx126x_set_rf_freq(&sx126x_context, 916800000);
    
    // Configurar PA para SX1262 (LP PA)
    sx126x_pa_cfg_params_t pa_cfg;
    pa_cfg.pa_duty_cycle = 0x04;
    pa_cfg.hp_max = 0x07;
    pa_cfg.device_sel = 0x00;  // SX1262
    pa_cfg.pa_lut = 0x01;
    sx126x_set_pa_cfg(&sx126x_context, &pa_cfg);
    
    // Configurar potencia de TX: 22 dBm
    sx126x_set_tx_params(&sx126x_context, 22, SX126X_RAMP_200_US);
    
    // Configurar buffer base address
    sx126x_set_buffer_base_address(&sx126x_context, 0, 0);
    
    RadioState = RF_IDLE;
}

void Radio_SetChannel(uint32_t freq) {
    sx126x_set_rf_freq(&sx126x_context, freq);
}

void Radio_SetTxConfig(uint8_t power, uint32_t bandwidth, uint8_t datarate, uint8_t coderate, uint16_t preambleLen, bool crcOn) {
    sx126x_mod_params_lora_t mod_params;
    mod_params.sf = (sx126x_lora_sf_t)datarate;
    mod_params.bw = (sx126x_lora_bw_t)bandwidth;
    mod_params.cr = (sx126x_lora_cr_t)coderate;
    mod_params.ldro = 0;
    
    sx126x_set_lora_mod_params(&sx126x_context, &mod_params);
    
    sx126x_pkt_params_lora_t pkt_params;
    pkt_params.preamble_len_in_symb = preambleLen;
    pkt_params.header_type = SX126X_LORA_PKT_EXPLICIT;
    pkt_params.pld_len_in_bytes = 255;
    pkt_params.crc_is_on = crcOn;
    pkt_params.invert_iq_is_on = false;
    
    sx126x_set_lora_pkt_params(&sx126x_context, &pkt_params);
    sx126x_set_tx_params(&sx126x_context, 22, SX126X_RAMP_200_US);  // Potencia máxima
}

void Radio_SetRxConfig(uint32_t bandwidth, uint8_t datarate, uint8_t coderate, uint16_t preambleLen, uint16_t symbTimeout, bool crcOn) {
    sx126x_mod_params_lora_t mod_params;
    mod_params.sf = (sx126x_lora_sf_t)datarate;
    mod_params.bw = (sx126x_lora_bw_t)bandwidth;
    mod_params.cr = (sx126x_lora_cr_t)coderate;
    mod_params.ldro = 0;
    
    sx126x_set_lora_mod_params(&sx126x_context, &mod_params);
    
    sx126x_pkt_params_lora_t pkt_params;
    pkt_params.preamble_len_in_symb = preambleLen;
    pkt_params.header_type = SX126X_LORA_PKT_EXPLICIT;
    pkt_params.pld_len_in_bytes = 255;
    pkt_params.crc_is_on = crcOn;
    pkt_params.invert_iq_is_on = true;  // Invertir IQ en RX
    
    sx126x_set_lora_pkt_params(&sx126x_context, &pkt_params);
    sx126x_set_lora_symb_nb_timeout(&sx126x_context, symbTimeout);
}

void Radio_Send(uint8_t* buffer, uint8_t size) {
    RadioState = RF_TX_RUNNING;
    
    // Poner en standby primero
    sx126x_set_standby(&sx126x_context, SX126X_STANDBY_CFG_RC);
    
    // Verificar estado antes de transmitir
    sx126x_chip_status_t chip_status;
    sx126x_get_status(&sx126x_context, &chip_status);
    
    // Actualizar longitud del paquete
    sx126x_pkt_params_lora_t pkt_params;
    pkt_params.preamble_len_in_symb = 8;
    pkt_params.header_type = SX126X_LORA_PKT_EXPLICIT;
    pkt_params.pld_len_in_bytes = size;
    pkt_params.crc_is_on = true;
    pkt_params.invert_iq_is_on = false;
    sx126x_set_lora_pkt_params(&sx126x_context, &pkt_params);
    
    // Escribir buffer
    sx126x_write_buffer(&sx126x_context, 0, buffer, size);
    
    // Configurar DIO1 para TX Done
    sx126x_set_dio_irq_params(&sx126x_context, 
                              SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                              SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                              SX126X_IRQ_NONE,
                              SX126X_IRQ_NONE);
    
    // Transmitir (timeout 5 segundos)
    sx126x_set_tx(&sx126x_context, 5000);
    
    // Esperar TX done (polling de DIO1 - simplificado)
    delay(1500);  // SF12 tarda más - aprox 1.3 segundos
    
    // Volver a standby para liberar BUSY
    sx126x_set_standby(&sx126x_context, SX126X_STANDBY_CFG_RC);
    delay(10);  // Dar tiempo a que BUSY se libere
    
    RadioState = RF_IDLE;
    if (RadioEvents != NULL && RadioEvents->TxDone != NULL) {
        RadioEvents->TxDone();
    }
}

void Radio_StartRx(uint32_t timeout) {
    RadioState = RF_RX_RUNNING;
    
    if (timeout == 0) {
        sx126x_set_rx(&sx126x_context, 0xFFFFFF);  // RX continuo
    } else {
        sx126x_set_rx(&sx126x_context, timeout);
    }
    
    // Polling simplificado de DIO1 (en producción usar interrupción)
    // Por ahora timeout simulado
    delay(timeout > 0 ? timeout : 5000);
    
    RadioState = RF_IDLE;
    if (RadioEvents != NULL && RadioEvents->RxTimeout != NULL) {
        RadioEvents->RxTimeout();
    }
}

void Radio_Standby(void) {
    sx126x_set_standby(&sx126x_context, SX126X_STANDBY_CFG_RC);
    RadioState = RF_IDLE;
}

RadioState_t Radio_GetStatus(void) {
    return RadioState;
}
