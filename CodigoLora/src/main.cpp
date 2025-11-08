#include <Arduino.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include "lora_config.h"
#include "definitions.h"

//Para el lora nuestro hay que compilar con [env:wio_lora].
//Compilar + subir: pio run -e wio_lora -t upload
//Monitor serie: pio device monitor -e wio_lora
//Si la placa real es otra (ej. wio_terminal, EDU‑CIAA con distinto core), cambia el board en platformio.ini al correcto.


// Serial buffer
static char serialBuf[BUFFER_SIZE];
static uint16_t serialPos = 0;
static bool haveLine = false;

// LMIC callbacks to provide keys from PROGMEM
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }

// Evento LMIC
void onEvent (ev_t ev) {
    switch(ev) {
        case EV_JOINED:
            Serial.println(F("EV_JOINED: Joined network"));
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes RX)"));
            if (LMIC.dataLen) {
                Serial.print(F("Received downlink: "));
                for (int i=0; i<LMIC.dataLen; i++) Serial.print((char)LMIC.frame[i]);
                Serial.println();
            }
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        default:
            // otros eventos opcionales
            break;
    }
}

// Enqueue uplink (no confirmed)
void do_send(const uint8_t* data, uint8_t len) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("TX pending, skipping send"));
        return;
    }
    // puerto 1, no confirmado
    LMIC_setTxData2(1, (xref2u1_t)data, len, 0);
    Serial.println(F("Uplink queued"));
}

void setup() {
    Serial.begin(EDUCIAA_BAUDRATE);
    while (!Serial) { delay(10); }

    Serial.println(F("Starting LMIC (OTAA)"));

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Optional: adjust frequency / tx power through LMIC if needed
    // LMIC_setDrTxpow(DR_SF7, TX_POWER); // ejemplo, LMIC_setDrTxpow puede variar por versión

    // Start joining
    LMIC_startJoining();
    Serial.println(F("Joining started..."));
}

void readSerialFromEDUCiaa() {
    while (Serial.available() > 0 && !haveLine) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialPos > 0) {
                serialBuf[serialPos] = '\0';
                haveLine = true;
            }
            // ignore extra CR/LF
        } else {
            if (serialPos < BUFFER_SIZE - 1) {
                serialBuf[serialPos++] = c;
            }
        }
    }
}

void loop() {
    // Procesa LMIC
    os_runloop_once();

    // Lee serial de la EDUCIAA
    readSerialFromEDUCiaa();

    // Si tenemos línea completa, la enviamos por LoRaWAN
    if (haveLine) {
        Serial.print(F("Sending line: "));
        Serial.println(serialBuf);
        do_send((const uint8_t*)serialBuf, strlen(serialBuf));
        // reset buffer
        serialPos = 0;
        haveLine = false;
        memset(serialBuf, 0, BUFFER_SIZE);
    }
}