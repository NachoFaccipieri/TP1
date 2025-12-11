#include "LoRaMac.h"
#include "radio.h"
#include "sapi.h"
#include "aes.h"
#include <string.h>

static LoRaMacPrimitives_t* MacPrimitives = NULL;
static LoRaMacCredentials_t MacCredentials;
static bool IsNetworkJoined = false;

// Sesión (después del JOIN)
static uint32_t DevAddr = 0;
static uint8_t NwkSKey[16];
static uint8_t AppSKey[16];
static uint16_t UpLinkCounter = 0;

// DevNonce para JOIN
static uint16_t DevNonce = 0;

// Callbacks del radio
static void OnRadioTxDone(void);
static void OnRadioRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
static void OnRadioTxTimeout(void);
static void OnRadioRxTimeout(void);
static void OnRadioRxError(void);

static RadioEvents_t RadioEvents = {
    .TxDone = OnRadioTxDone,
    .TxTimeout = OnRadioTxTimeout,
    .RxDone = OnRadioRxDone,
    .RxTimeout = OnRadioRxTimeout,
    .RxError = OnRadioRxError
};

LoRaMacStatus_t LoRaMac_Init(LoRaMacPrimitives_t* primitives, LoRaMacCredentials_t* credentials) {
    if (primitives == NULL || credentials == NULL) {
        return LORAMAC_STATUS_PARAMETER_INVALID;
    }
    
    MacPrimitives = primitives;
    memcpy(&MacCredentials, credentials, sizeof(LoRaMacCredentials_t));
    
    // Inicializar radio
    Radio_Init(&RadioEvents);
    
    uartWriteString(UART_USB, "Radio inicializado OK\r\n");
    
    // AU915 Sub-band 2: Canal 8 inicial (916.8 MHz)
    Radio_SetChannel(916800000);
    
    // SF12 (DR0), BW125, CR4/5 - Más robusto
    Radio_SetTxConfig(14, 4, 12, 1, 8, true);  // 14dBm, BW125, SF12, CR4/5
    Radio_SetRxConfig(4, 12, 1, 8, 10, true);
    
    IsNetworkJoined = false;
    DevNonce = 0;
    UpLinkCounter = 0;
    
    return LORAMAC_STATUS_OK;
}

LoRaMacStatus_t LoRaMac_Join(void) {
    // AU915 Sub-band 2: Canales 8-15 (UPLINK CORRECTO)
    static uint32_t au915_channels[] = {
        916800000, 917000000, 917200000, 917400000,
        917600000, 917800000, 918000000, 918200000
    };
    static uint8_t channel_index = 0;
    
    // Rotar entre canales en cada intento
    uint32_t freq = au915_channels[channel_index];
    Radio_SetChannel(freq);
    
    char freq_str[50];
    sprintf(freq_str, "Canal %d: %.1f MHz\r\n", 8 + channel_index, freq / 1000000.0);
    uartWriteString(UART_USB, freq_str);
    
    channel_index = (channel_index + 1) % 8;
    
    uint8_t joinReq[23];
    
    // JOIN REQUEST structure:
    // MHDR(1) | JoinEUI(8) | DevEUI(8) | DevNonce(2) | MIC(4)
    
    // MHDR: Join Request
    joinReq[0] = 0x00;  // MHDR para Join Request
    
    // JoinEUI (little endian)
    for (int i = 0; i < 8; i++) {
        joinReq[1 + i] = MacCredentials.JoinEui[7 - i];
    }
    
    // DevEUI (little endian)
    for (int i = 0; i < 8; i++) {
        joinReq[9 + i] = MacCredentials.DevEui[7 - i];
    }
    
    // DevNonce (little endian)
    DevNonce++;
    joinReq[17] = DevNonce & 0xFF;
    joinReq[18] = (DevNonce >> 8) & 0xFF;
    
    // Calcular MIC con AES-CMAC (AppKey, MHDR+JoinEUI+DevEUI+DevNonce)
    uint8_t mic[16];
    aes_cmac(MacCredentials.AppKey, joinReq, 19, mic);
    
    // Copiar solo los primeros 4 bytes del MIC
    memcpy(&joinReq[19], mic, 4);
    
    uartWriteString(UART_USB, "Enviando JOIN REQUEST...\r\n");
    uartWriteString(UART_USB, "  SF12, BW125\r\n");
    uartWriteString(UART_USB, "  Size: 23 bytes\r\n");
    
    // Enviar JOIN REQUEST
    Radio_Send(joinReq, 23);
    
    // NO esperar RX por ahora - solo testear si el gateway recibe
    uartWriteString(UART_USB, "TX completado\r\n");
    
    return LORAMAC_STATUS_OK;
}

LoRaMacStatus_t LoRaMac_Send(LoRaMacTxInfo_t* txInfo) {
    if (!IsNetworkJoined) {
        return LORAMAC_STATUS_NO_NETWORK_JOINED;
    }
    
    if (txInfo == NULL || txInfo->Buffer == NULL || txInfo->BufferSize == 0) {
        return LORAMAC_STATUS_PARAMETER_INVALID;
    }
    
    // DATA UPLINK structure (simplificado):
    // MHDR(1) | DevAddr(4) | FCtrl(1) | FCnt(2) | FPort(1) | Payload | MIC(4)
    
    uint8_t frame[255];
    uint8_t frameSize = 0;
    
    // MHDR: Unconfirmed Data Up
    frame[frameSize++] = txInfo->Confirmed ? 0x80 : 0x40;
    
    // DevAddr (little endian)
    frame[frameSize++] = DevAddr & 0xFF;
    frame[frameSize++] = (DevAddr >> 8) & 0xFF;
    frame[frameSize++] = (DevAddr >> 16) & 0xFF;
    frame[frameSize++] = (DevAddr >> 24) & 0xFF;
    
    // FCtrl
    frame[frameSize++] = 0x00;
    
    // FCnt (little endian)
    frame[frameSize++] = UpLinkCounter & 0xFF;
    frame[frameSize++] = (UpLinkCounter >> 8) & 0xFF;
    
    // FPort
    frame[frameSize++] = txInfo->FPort;
    
    // Payload (sin encriptar por ahora)
    memcpy(&frame[frameSize], txInfo->Buffer, txInfo->BufferSize);
    frameSize += txInfo->BufferSize;
    
    // MIC (simplificado - zeros por ahora)
    memset(&frame[frameSize], 0x00, 4);
    frameSize += 4;
    
    UpLinkCounter++;
    
    // Enviar
    Radio_Send(frame, frameSize);
    
    return LORAMAC_STATUS_OK;
}

bool LoRaMac_IsJoined(void) {
    return IsNetworkJoined;
}

void LoRaMac_Process(void) {
    // Procesar eventos pendientes (simplificado)
}

// Callbacks del Radio
static void OnRadioTxDone(void) {
    uartWriteString(UART_USB, "TX Done\r\n");
    
    if (MacPrimitives != NULL && MacPrimitives->MacMcpsConfirm != NULL) {
        MacPrimitives->MacMcpsConfirm(LORAMAC_EVENT_INFO_STATUS_OK);
    }
}

static void OnRadioRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    uartWriteString(UART_USB, "RX Done\r\n");
    
    // Si es JOIN ACCEPT
    if (!IsNetworkJoined && size >= 17) {
        uartWriteString(UART_USB, "JOIN ACCEPT recibido!\r\n");
        
        // Extraer DevAddr (simplificado - sin descifrar)
        // En producción descifrar con AppKey
        DevAddr = 0x12345678;  // Dummy por ahora
        IsNetworkJoined = true;
        
        if (MacPrimitives != NULL && MacPrimitives->MacMlmeConfirm != NULL) {
            MacPrimitives->MacMlmeConfirm(LORAMAC_EVENT_INFO_STATUS_OK);
        }
    } else if (IsNetworkJoined) {
        // Downlink
        if (MacPrimitives != NULL && MacPrimitives->MacMcpsIndication != NULL) {
            MacPrimitives->MacMcpsIndication(payload, size, rssi, snr);
        }
    }
}

static void OnRadioTxTimeout(void) {
    uartWriteString(UART_USB, "TX Timeout\r\n");
}

static void OnRadioRxTimeout(void) {
    uartWriteString(UART_USB, "RX Timeout\r\n");
    
    if (!IsNetworkJoined) {
        // JOIN falló
        if (MacPrimitives != NULL && MacPrimitives->MacMlmeConfirm != NULL) {
            MacPrimitives->MacMlmeConfirm(LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL);
        }
    }
}

static void OnRadioRxError(void) {
    uartWriteString(UART_USB, "RX Error\r\n");
}
