#ifndef __LORAMAC_TYPES_H__
#define __LORAMAC_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

// Estados del LoRaMac
typedef enum {
    LORAMAC_STATUS_OK = 0,
    LORAMAC_STATUS_ERROR,
    LORAMAC_STATUS_BUSY,
    LORAMAC_STATUS_SERVICE_UNKNOWN,
    LORAMAC_STATUS_PARAMETER_INVALID,
    LORAMAC_STATUS_NO_NETWORK_JOINED,
} LoRaMacStatus_t;

// Eventos del LoRaMac
typedef enum {
    LORAMAC_EVENT_INFO_STATUS_OK = 0,
    LORAMAC_EVENT_INFO_STATUS_ERROR,
    LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT,
    LORAMAC_EVENT_INFO_STATUS_RX1_TIMEOUT,
    LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT,
    LORAMAC_EVENT_INFO_STATUS_RX1_ERROR,
    LORAMAC_EVENT_INFO_STATUS_RX2_ERROR,
    LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL,
    LORAMAC_EVENT_INFO_STATUS_DOWNLINK_REPEATED,
    LORAMAC_EVENT_INFO_STATUS_TX_DR_PAYLOAD_SIZE_ERROR,
} LoRaMacEventInfoStatus_t;

// Tipo de mensaje
typedef enum {
    LORAMAC_MSG_TYPE_JOIN_REQUEST = 0x00,
    LORAMAC_MSG_TYPE_JOIN_ACCEPT = 0x01,
    LORAMAC_MSG_TYPE_DATA_UNCONFIRMED_UP = 0x02,
    LORAMAC_MSG_TYPE_DATA_UNCONFIRMED_DOWN = 0x03,
    LORAMAC_MSG_TYPE_DATA_CONFIRMED_UP = 0x04,
    LORAMAC_MSG_TYPE_DATA_CONFIRMED_DOWN = 0x05,
} LoRaMacMsgType_t;

// Estructura de callbacks
typedef struct {
    void (*MacMcpsConfirm)(uint8_t status);
    void (*MacMcpsIndication)(uint8_t* payload, uint8_t size, int16_t rssi, int8_t snr);
    void (*MacMlmeConfirm)(uint8_t status);
} LoRaMacPrimitives_t;

// Configuración de credenciales
typedef struct {
    uint8_t DevEui[8];
    uint8_t JoinEui[8];
    uint8_t AppKey[16];
} LoRaMacCredentials_t;

// Parámetros de transmisión
typedef struct {
    uint8_t* Buffer;
    uint8_t BufferSize;
    uint8_t FPort;
    bool Confirmed;
} LoRaMacTxInfo_t;

#endif // __LORAMAC_TYPES_H__
