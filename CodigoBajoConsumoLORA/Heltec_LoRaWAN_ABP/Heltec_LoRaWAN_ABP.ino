/*
 * HELTEC LoRaWAN ABP - Modo Esclavo Bajo Consumo
 * Se despierta por pin externo, transmite y se vuelve a dormir.
 */

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// --- CONFIGURACION DE BAJO CONSUMO ---
#define WAKEUP_PIN      GPIO_NUM_33 // Pin que recibe la señal de la EDU-CIAA
#define WAKEUP_LEVEL    1           // Despertar cuando el pin sea ALTO (High)

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
            Serial.println(F("✓ TX completo. Entrando en DEEP SLEEP."));
            
            // Esperar a que se vacíe el buffer serial antes de cortar
            Serial.flush();
            Serial2.flush();

            // Configurar el despertar por PIN externo (GPIO 33)
            // Se despierta cuando el pin se pone en 1 (HIGH)
            esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, WAKEUP_LEVEL);
            
            // Iniciar modo de bajo consumo profundo
            // El procesador se apaga y al despertar reinicia el programa desde setup()
            esp_deep_sleep_start();
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
    
    // Verificar motivo del despertar
    if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0){
        Serial.println(F("\n=== DESPERTADO POR EDU-CIAA ==="));
    } else {
        Serial.println(F("\n=== ARRANQUE INICIAL / RESET ==="));
    }
    
    // Configuración UART con EDU-CIAA
    Serial2.begin(9600, SERIAL_8N1, 4, 17); // Bajamos a 9600 para mayor estabilidad
    Serial2.setTimeout(2000); // 2 segundos de espera máxima

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
    
    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7, 14);
    
    Serial.println(F("✓ Esperando datos de EDU-CIAA..."));
}

void loop() {
    // Al despertar, ejecutamos LMIC una vez para gestión interna
    os_runloop_once();
    
    // Esperamos datos de la EDU-CIAA
    // Como la EDU-CIAA nos despierta y LUEGO manda datos, leemos directo
    if (Serial2.available()) {
        String linea = Serial2.readStringUntil('\n');
        
        if (linea.length() > 0 && !(LMIC.opmode & OP_TXRXPEND)) {
            Serial.print(F("RX UART: "));
            Serial.println(linea);
            
            // Preparar Payload
            uint8_t payload[linea.length() + 1];
            linea.getBytes(payload, linea.length() + 1);
            
            // Encolar transmisión
            // Al terminar la transmisión, el evento EV_TXCOMPLETE nos mandará a dormir
            LMIC_setTxData2(1, payload, linea.length(), 0);
            Serial.println(F("-> Encolado para TX LoRaWAN"));
        }
    }
    // Si pasa mucho tiempo sin recibir nada (error de sincro), volvemos a dormir
    // para no gastar batería. (Timeout de seguridad de 10 segundos)
    if (millis() > 10000 && !(LMIC.opmode & OP_TXRXPEND)) {
        Serial.println(F("Timeout esperando UART. Durmiendo..."));
        esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, WAKEUP_LEVEL);
        esp_deep_sleep_start();
    }
}