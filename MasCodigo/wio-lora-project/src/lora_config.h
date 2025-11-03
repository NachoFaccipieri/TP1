#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>
#include <stdint.h>

// Parámetros de radio (ajustá si corresponde a tu región)
#define LORA_FREQUENCY 915E6
#define TX_POWER 14

// --- RELLENÁ ESTAS CONSTANTES CON TUS CREDENCIALES (OTAA) ---
// IMPORTANTE: LMIC espera los bytes en orden LSB (little-endian).
// Si tus credenciales están en formato habitual (MSB, ej "00-11-22-33-44-55-66-77")
// hay que invertir el orden al pegarlas aquí.

// Ejemplo vacío — reemplazá los 0x00 por tus valores en LSB order:
static const uint8_t DEVEUI[8] PROGMEM  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8_t APPEUI[8] PROGMEM  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const uint8_t APPKEY[16] PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

#endif