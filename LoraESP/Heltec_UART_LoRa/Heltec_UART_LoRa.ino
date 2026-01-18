/*
 * HELTEC: Recibe por UART -> Transmite por LoRa
 * 
 * Recibe datos desde EDU-CIAA por UART (RX)
 * Transmite al Gateway RAK por LoRa (917.0 MHz)
 */

#include <LoRa.h>

// Pines LoRa del Heltec V2
#define LORA_SCK     5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_SS     18
#define LORA_RST    14
#define LORA_DIO0   26

#define LORA_BAND   917.0E6  // Frecuencia Gateway RAK

void setup() {
  // Serial para debug por USB
  Serial.begin(115200);
  delay(1000);
  
  // Serial2 para recibir de EDU-CIAA (pines 16=RX2, 17=TX2)
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  
  Serial.println("\n=== HELTEC: UART2 -> LoRa ===");
  Serial.println("Inicializando LoRa...");
  
  // Inicializar LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("ERROR: LoRa no inicio");
    while (1);
  }
  
  // Configurar LoRa para RAK
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x34);
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  
  Serial.println("OK: LoRa 917.0 MHz, SF7");
  Serial.println("Esperando datos en pin 16 (RX2)...\n");
}

void loop() {
  // Si hay datos desde EDU-CIAA por Serial2
  if (Serial2.available()) {
    String linea = Serial2.readStringUntil('\n');
    
    if (linea.length() > 0) {
      // Mostrar en Serial USB (debug)
      Serial.print("RX: ");
      Serial.println(linea);
      
      // Transmitir por LoRa al Gateway
      LoRa.beginPacket();
      LoRa.print(linea);
      LoRa.endPacket();
      
      Serial.println("  -> TX LoRa OK\n");
      
      delay(100);
    }
  }
}
