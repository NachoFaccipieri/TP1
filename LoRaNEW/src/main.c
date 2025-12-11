#include "sapi.h"
#include "LoRaMac.h"
#include "LoRaMacTypes.h"
#include "sx126x.h"
#include "sx126x_hal.h"
#include "sx126x_hal_context.h"
#include <string.h>

// Implementación de HAL functions para sx126x usando SPI hardware
// --- REEMPLAZAR EN MAIN.C ---

sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length) {
    const sx126x_hal_context_t* ctx = (const sx126x_hal_context_t*)context;
    
    // TIMEOUT DE SEGURIDAD: Esperar máx 2000ms
    uint32_t timeout = 0;
    while (gpioRead(ctx->busy_pin) == HIGH) {
        delay(1);
        timeout++;
        if (timeout > 2000) {
             uartWriteString(UART_USB, "[ERROR] BUSY PIN TRABADO (Check Cables!)\r\n");
             return SX126X_HAL_STATUS_ERROR; // Salir para no colgar
        }
    }
    
    gpioWrite(ctx->nss_pin, LOW);
    spiWrite(SPI0, (uint8_t*)command, command_length);
    
    if (data_length > 0) {
        spiWrite(SPI0, (uint8_t*)data, data_length);
    }
    
    gpioWrite(ctx->nss_pin, HIGH);
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length) {
    const sx126x_hal_context_t* ctx = (const sx126x_hal_context_t*)context;
    
    // TIMEOUT DE SEGURIDAD
    uint32_t timeout = 0;
    while (gpioRead(ctx->busy_pin) == HIGH) {
        delay(1);
        timeout++;
        if (timeout > 2000) {
             uartWriteString(UART_USB, "[ERROR] BUSY PIN TRABADO EN LECTURA\r\n");
             return SX126X_HAL_STATUS_ERROR;
        }
    }
    
    gpioWrite(ctx->nss_pin, LOW);
    spiWrite(SPI0, (uint8_t*)command, command_length);
    
    if (data_length > 0) {
        spiRead(SPI0, data, data_length);
    }
    
    gpioWrite(ctx->nss_pin, HIGH);
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset(const void* context) {
    const sx126x_hal_context_t* ctx = (const sx126x_hal_context_t*)context;
    
    gpioWrite(ctx->reset_pin, LOW);
    delay(10);
    gpioWrite(ctx->reset_pin, HIGH);
    delay(10);
    
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void* context) {
    const sx126x_hal_context_t* ctx = (const sx126x_hal_context_t*)context;
    
    gpioWrite(ctx->nss_pin, LOW);
    delay(1);
    gpioWrite(ctx->nss_pin, HIGH);
    
    return SX126X_HAL_STATUS_OK;
}

// Credenciales LoRaWAN
static LoRaMacCredentials_t credentials = {
    .DevEui = {0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x11, 0x11},
    .JoinEui = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .AppKey = {0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 
               0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00}
};

static bool joinSuccess = false;

// Callbacks de LoRaMac
void OnMacMcpsConfirm(uint8_t status) {
    if (status == LORAMAC_EVENT_INFO_STATUS_OK) {
        uartWriteString(UART_USB, "Uplink OK\r\n");
    } else {
        uartWriteString(UART_USB, "Uplink ERROR\r\n");
    }
}

void OnMacMcpsIndication(uint8_t* payload, uint8_t size, int16_t rssi, int8_t snr) {
    uartWriteString(UART_USB, "Downlink recibido!\r\n");
}

void OnMacMlmeConfirm(uint8_t status) {
    if (status == LORAMAC_EVENT_INFO_STATUS_OK) {
        uartWriteString(UART_USB, "JOIN SUCCESS! Conectado al gateway\r\n");
        joinSuccess = true;
    } else {
        uartWriteString(UART_USB, "JOIN FAILED - Reintentando en 10s...\r\n");
        joinSuccess = false;
    }
}

int main(void) {
    boardInit();
    uartConfig(UART_USB, 115200);
    spiInit(SPI0);
    
    uartWriteString(UART_USB, "\r\n=== LoRaWAN OTAA Test ===\r\n");
    uartWriteString(UART_USB, "DevEUI: 00:00:11:11:00:00:11:11\r\n");
    uartWriteString(UART_USB, "Frecuencia: AU915 Sub-band 2 (916.8-918.2 MHz)\r\n\r\n");
    
    // Primitivas de LoRaMac
    LoRaMacPrimitives_t primitives = {
        .MacMcpsConfirm = OnMacMcpsConfirm,
        .MacMcpsIndication = OnMacMcpsIndication,
        .MacMlmeConfirm = OnMacMlmeConfirm
    };
    
    // Inicializar LoRaMac
    LoRaMac_Init(&primitives, &credentials);
    
    uartWriteString(UART_USB, "LoRaMac inicializado\r\n");
    
    // Intentar JOIN
    int joinAttempts = 0;
    while (!joinSuccess && joinAttempts < 10) {
        joinAttempts++;
        uartWriteString(UART_USB, "Intento de JOIN #");
        char buffer[10];
        sprintf(buffer, "%d", joinAttempts);
        uartWriteString(UART_USB, buffer);
        uartWriteString(UART_USB, "\r\n");
        
        LoRaMac_Join();
        delay(10000);  // Esperar 10 segundos entre intentos
    }
    
    if (!joinSuccess) {
        uartWriteString(UART_USB, "ERROR: No se pudo conectar al gateway\r\n");
        while(1) {
            delay(1000);
        }
    }
    
    // Loop principal - enviar datos cada 30 segundos
    int contador = 0;
    while(1) {
        if (LoRaMac_IsJoined()) {
            contador++;
            
            char payload[50];
            sprintf(payload, "Hola RAK %d", contador);
            
            LoRaMacTxInfo_t txInfo = {
                .Buffer = (uint8_t*)payload,
                .BufferSize = strlen(payload),
                .FPort = 1,
                .Confirmed = false
            };
            
            uartWriteString(UART_USB, "Enviando: ");
            uartWriteString(UART_USB, payload);
            uartWriteString(UART_USB, "\r\n");
            
            LoRaMac_Send(&txInfo);
        }
        
        delay(30000);  // Enviar cada 30 segundos
    }
    
    return 0;
}
