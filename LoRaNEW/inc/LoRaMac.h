#ifndef __LORAMAC_H__
#define __LORAMAC_H__

#include "LoRaMacTypes.h"

// Inicializar LoRaMac
LoRaMacStatus_t LoRaMac_Init(LoRaMacPrimitives_t* primitives, LoRaMacCredentials_t* credentials);

// Hacer JOIN OTAA
LoRaMacStatus_t LoRaMac_Join(void);

// Enviar datos
LoRaMacStatus_t LoRaMac_Send(LoRaMacTxInfo_t* txInfo);

// Verificar si está joined
bool LoRaMac_IsJoined(void);

// Procesar eventos (llamar en loop)
void LoRaMac_Process(void);

#endif // __LORAMAC_H__
