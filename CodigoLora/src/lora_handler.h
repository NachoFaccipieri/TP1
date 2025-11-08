#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>

// Function to initialize the LoRa module
void initLoRa();

// Function to send data via LoRa
void sendLoRaData(const String &data);

// Function to receive data via LoRa
String receiveLoRaData();

#endif // LORA_HANDLER_H