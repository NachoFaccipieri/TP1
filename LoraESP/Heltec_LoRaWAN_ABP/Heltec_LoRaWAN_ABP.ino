/*
 * HELTEC LoRaWAN ABP - Recibe UART y transmite al RAK
 * 
 * Modo ABP (sin join, claves fijas)
 */

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// ABP keys (MSB format)
static const u1_t PROGMEM NWKSKEY[16] = { 0xd3, 0x1a, 0xe8, 0x70, 0xa6, 0x45, 0x86, 0x70, 0x86, 0xb6, 0x12, 0x8e, 0x9c, 0xa1, 0x5d, 0x2a };
static const u1_t PROGMEM APPSKEY[16] = { 0x20, 0x3b, 0x26, 0x4a, 0x9f, 0x0b, 0x87, 0x1d, 0x95, 0x11, 0xef, 0x3c, 0x25, 0x61, 0xe8, 0x37 };
static const u4_t DEVADDR = 0x01234567;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// Pin mapping Heltec LoRa 32 V2
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 35, 34},
};

static osjob_t sendjob;

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("✓ TX completo"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("  ACK recibido"));
            break;
        case EV_TXSTART:
            Serial.println(F("TX iniciando..."));
            break;
        default:
            Serial.print(F("Event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println(F("\n=== HELTEC LoRaWAN ABP ==="));
    
    Serial2.begin(250, SERIAL_8N1, 4, 17);  // 300 baud ULTRA lento
    Serial2.setTimeout(4000);
    // Init LMIC
    os_init();
    LMIC_reset();
    
    // Configurar ABP
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    
    // Configurar canales AU915 sub-banda 2
    for(int i=0; i<72; i++) {
        LMIC_disableChannel(i);
    }
    for(int i=8; i<16; i++) {
        LMIC_enableChannel(i);
    }
    LMIC_enableChannel(65);
    
    // Deshabilitar link check
    LMIC_setLinkCheckMode(0);
    
    // Set data rate y TX power
    LMIC_setDrTxpow(DR_SF7, 14);
    
    Serial.println(F("✓ ABP configurado - listo para TX"));
}

void loop() {
    os_runloop_once();
    
    // Si hay datos desde EDU-CIAA
    if (Serial2.available()) {
        String linea = Serial2.readStringUntil('\n');
        
        if (linea.length() > 0 && !(LMIC.opmode & OP_TXRXPEND)) {
            Serial.print(F("RX UART: "));
            Serial.println(linea);
            
            // Enviar por LoRaWAN
            uint8_t payload[linea.length() + 1];
            linea.getBytes(payload, linea.length() + 1);
            
            LMIC_setTxData2(1, payload, linea.length(), 0);
            Serial.println(F("-> Encolado para TX LoRaWAN"));
        }
    }
}
