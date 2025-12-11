#ifndef __RADIO_H__
#define __RADIO_H__

#include <stdint.h>
#include <stdbool.h>

// Estados del radio
typedef enum {
    RF_IDLE = 0,
    RF_RX_RUNNING,
    RF_TX_RUNNING,
    RF_CAD,
} RadioState_t;

// Eventos del radio
typedef struct {
    void (*TxDone)(void);
    void (*TxTimeout)(void);
    void (*RxDone)(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
    void (*RxTimeout)(void);
    void (*RxError)(void);
} RadioEvents_t;

// Funciones de la capa Radio
void Radio_Init(RadioEvents_t* events);
void Radio_SetChannel(uint32_t freq);
void Radio_SetTxConfig(uint8_t power, uint32_t bandwidth, uint8_t datarate, uint8_t coderate, uint16_t preambleLen, bool crcOn);
void Radio_SetRxConfig(uint32_t bandwidth, uint8_t datarate, uint8_t coderate, uint16_t preambleLen, uint16_t symbTimeout, bool crcOn);
void Radio_Send(uint8_t* buffer, uint8_t size);
void Radio_StartRx(uint32_t timeout);
void Radio_Standby(void);
RadioState_t Radio_GetStatus(void);

#endif // __RADIO_H__
