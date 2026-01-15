/*
 * PASO 2: Heltec WiFi + LoRa RECEPTOR
 * 
 * Este código:
 * - Se conecta a WiFi
 * - Escucha mensajes LoRa en 915 MHz
 * - Muestra los datos recibidos en el Serial Monitor
 */

#include <WiFi.h>
#include <LoRa.h>

// ====== CONFIGURACIÓN WiFi ======
const char* ssid = "Tornado 1";
const char* password = "0Faccipieri0";

// ====== CONFIGURACIÓN LoRa - Pines Heltec V2 ======
#define LORA_SCK     5    // GPIO5  -- SX1278's SCK
#define LORA_MISO   19    // GPIO19 -- SX1278's MISO
#define LORA_MOSI   27    // GPIO27 -- SX1278's MOSI
#define LORA_SS     18    // GPIO18 -- SX1278's CS
#define LORA_RST    14    // GPIO14 -- SX1278's RESET
#define LORA_DIO0   26    // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define LORA_BAND   868.3E6 // Frecuencia 868.3 MHz (test)

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== HELTEC LORA 32 - RECEPTOR WiFi + LoRa ===");
  
  // ========== INICIALIZAR WiFi ==========
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi no conectó (continuando sin WiFi)");
  }
  
  // ========== INICIALIZAR LoRa ==========
  Serial.println("\nInicializando LoRa...");
  
  // Configurar pines SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  // Iniciar LoRa en 915 MHz
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("✗ Error: No se pudo inicializar LoRa");
    while (1); // Detener ejecución
  }
  
  // Configurar parámetros EXPLÍCITOS - IGUALES AL EJEMPLO
  Serial.println("Configurando parámetros LoRa...");
  LoRa.setSpreadingFactor(12);     // SF12 (como el ejemplo)
  LoRa.setSignalBandwidth(125E3);  // BW 125 kHz
  LoRa.setCodingRate4(5);          // CR 4/5
  LoRa.setPreambleLength(8);       // Preamble 8 símbolos
  LoRa.setSyncWord(0x21);          // Sync word 0x21 (como ejemplo)
  LoRa.enableCrc();                // CRC habilitado
  LoRa.setTxPower(14);             // Potencia 14 dBm
  
  // Probar sin inversión de IQ (por defecto)
  // Si no funciona, descomentar: LoRa.enableInvertIQ();
  
  Serial.println("✓ LoRa inicializado en 868.3 MHz");
  Serial.println("  SF12, BW125, CR4/5, Preamble=8, SyncWord=0x21");
  Serial.println("\n>>> Esperando mensajes LoRa...\n");
}

void loop() {
  // Verificar si llegó un paquete LoRa
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    // Leer el mensaje recibido
    String mensaje = "";
    while (LoRa.available()) {
      mensaje += (char)LoRa.read();
    }
    
    // Obtener RSSI (potencia de señal)
    int rssi = LoRa.packetRssi();
    
    // Mostrar en Serial Monitor
    Serial.println("========== MENSAJE RECIBIDO ==========");
    Serial.print("Datos: ");
    Serial.println(mensaje);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println("======================================\n");
    
    // TODO: Aquí después vamos a guardar en MongoDB
  }
  
  // Pequeña pausa
  delay(10);
}
