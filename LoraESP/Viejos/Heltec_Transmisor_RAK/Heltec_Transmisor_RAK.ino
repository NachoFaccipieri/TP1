/*
 * HELTEC TRANSMISOR - PRUEBA CON GATEWAY RAK
 * 
 * Proyecto nuevo para probar:
 * - Heltec ESP32 con SX1278 como TRANSMISOR
 * - Gateway RAK como RECEPTOR
 * - Frecuencia: 917.0 MHz (banda RAK)
 */

#include <LoRa.h>

// ====== CONFIGURACIÓN LoRa - Pines Heltec V2 ======
#define LORA_SCK     5    // GPIO5  -- SX1278's SCK
#define LORA_MISO   19    // GPIO19 -- SX1278's MISO
#define LORA_MOSI   27    // GPIO27 -- SX1278's MOSI
#define LORA_SS     18    // GPIO18 -- SX1278's CS
#define LORA_RST    14    // GPIO14 -- SX1278's RESET
#define LORA_DIO0   26    // GPIO26 -- SX1278's IRQ

#define LORA_BAND   917.0E6 // Frecuencia 917.0 MHz

int contador = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== HELTEC -> RAK GATEWAY (PRUEBA) ===");
  
  // Configurar pines SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  // Iniciar LoRa
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("✗ Error inicializando LoRa");
    while (1);
  }
  
  // Configurar parámetros
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x34);
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  
  Serial.println("✓ LoRa OK - 917.0 MHz, SF7, BW125");
  Serial.println(">>> Transmitiendo cada 5 segundos...\n");
}

void loop() {
  // TRANSMISIÓN DETENIDA
  Serial.println("*** Transmisión pausada ***");
  delay(10000);
  
  /*
  String mensaje = "TEST_" + String(contador);
  
  Serial.print("[TX " + String(contador) + "] ");
  Serial.println(mensaje);
  
  LoRa.beginPacket();
  LoRa.print(mensaje);
  LoRa.endPacket();
  
  Serial.println("  -> Enviado\n");
  
  contador++;
  delay(5000);
  */
}
