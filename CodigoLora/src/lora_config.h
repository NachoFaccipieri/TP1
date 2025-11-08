#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>

// Credenciales proporcionadas por el usuario.
// Nota: LMIC suele requerir APPEUI y DEVEUI en little-endian (LSB first).
// Si las tenés en el orden "humano" (MSB first), invertí el orden de bytes para APPEUI/DEVEUI.

// AppKey (MSB first) - dejar tal cual
static const uint8_t APPKEY[16] PROGMEM = {
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00,
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00
};

// AppEUI / JoinEUI (LMIC: little-endian). Usuario dio todos ceros => igual en cualquier orden.
static const uint8_t APPEUI[8] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// DevEUI (LMIC: little-endian). Usuario original: {0x00,0x00,0x11,0x11,0x00,0x00,0x11,0x11}
// Reordenado LSB-first para LMIC:
static const uint8_t DEVEUI[8] PROGMEM = { 0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00 };

// LMIC callback helpers (las bibliotecas LMIC llaman a estas funciones)
void os_getArtEui (uint8_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (uint8_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (uint8_t* buf) { memcpy_P(buf, APPKEY, 16); }

#endif // LORA_CONFIG_H