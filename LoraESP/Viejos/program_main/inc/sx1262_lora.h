/*
 * SX1262 LoRa Driver para EDU-CIAA
 * Driver simple para transmitir datos por LoRa
 */

#ifndef _SX1262_LORA_H_
#define _SX1262_LORA_H_

#include "sapi.h"
#include <stdint.h>
#include <stdbool.h>

// Comandos SX1262
#define SX1262_CMD_SET_STANDBY          0x80
#define SX1262_CMD_SET_TX               0x83
#define SX1262_CMD_WRITE_BUFFER         0x0E
#define SX1262_CMD_SET_PACKET_PARAMS    0x8C
#define SX1262_CMD_SET_MODULATION       0x8B
#define SX1262_CMD_SET_RF_FREQUENCY     0x86
#define SX1262_CMD_SET_PA_CONFIG        0x95
#define SX1262_CMD_SET_DIO2_AS_RF_SW    0x9D

// Pines de conexión del Wio-SX1262 (según tu PCB)
#define SX1262_NSS      GPIO5  // Pin 36
#define SX1262_RST      GPIO3  // Pin 34
#define SX1262_BUSY     GPIO1  // Pin 32

// Funciones públicas
void sx1262_init(void);
void sx1262_transmit(const char* data, uint8_t length);

#endif /* _SX1262_LORA_H_ */
