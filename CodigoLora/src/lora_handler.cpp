#include "lora_handler.h"
#include "definitions.h"
#include <LoRa.h>

void initLoRa() {
    // usa las macros definidas en definitions.h
    LoRa.begin((long)LORA_FREQUENCY);
    LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    LoRa.setSignalBandwidth((long)LORA_BANDWIDTH);
    LoRa.setTxPower(LORA_TX_POWER);
}

void sendLoRaData(const String &message) {
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
}

String receiveLoRaData() {
    String message = "";
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        while (LoRa.available()) {
            message += (char)LoRa.read();
        }
    }
    return message;
}