#include "lora_handler.h"
#include <LoRa.h>

void initLoRa() {
    LoRa.begin(FREQUENCY);
    LoRa.setSpreadingFactor(SPREADING_FACTOR);
    LoRa.setSignalBandwidth(SIGNAL_BANDWIDTH);
    LoRa.setTxPower(TX_POWER);
}

void sendLoRaMessage(const String &message) {
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
}

String receiveLoRaMessage() {
    String message = "";
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        while (LoRa.available()) {
            message += (char)LoRa.read();
        }
    }
    return message;
}