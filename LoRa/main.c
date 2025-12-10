/*==============================================================================
 * PROYECTO: Comunicación LoRaWAN con EDU-CIAA y SX1262
 *============================================================================*/

#include "sapi.h"              // Librería de EDU-CIAA
#include "radio.h"             // Interfaz de radio (conecta SX1262 con LoRaWAN)
#include "LoRaMac.h"           // Stack LoRaWAN
#include "region/Region.h"     // Configuración AU915 MHz

/*==============================================================================
 * CONFIGURACIÓN DE PINES - SX1262 conectado a EDU-CIAA
 *============================================================================*/
#define RADIO_NSS       GPIO0    // Chip Select
#define RADIO_BUSY      GPIO1    // Pin BUSY del SX1262
#define RADIO_NRESET    GPIO2    // Pin de RESET
#define RADIO_DIO_1     GPIO3    // Pin de interrupción

/*==============================================================================
 * CREDENCIALES LoRaWAN - Configuradas en el Gateway RAK7268
 *============================================================================*/
// DevEUI: Identificador único del dispositivo (8 bytes en formato LSB)
static const uint8_t LoRaWAN_DevEUI[8] = { 
    0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x11, 0x11 
};

// JoinEUI (AppEUI): Identificador de la aplicación (8 bytes en formato LSB)
static const uint8_t LoRaWAN_JoinEUI[8] = { 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

// AppKey: Clave de cifrado (16 bytes en formato MSB)
static const uint8_t LoRaWAN_AppKey[16] = {
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00,
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00
};

/*==============================================================================
 * CONFIGURACIÓN DE LA APLICACIÓN
 *============================================================================*/
#define puertoLoraWAN    15              // Puerto de la aplicación
#define intervalo      20000             // Enviar cada 20 segundos

/*==============================================================================
 * VARIABLES GLOBALES
 *============================================================================*/
// Buffer de datos a enviar
static uint8_t AppDataBuffer[64];          // Buffer de hasta 64 bytes
static uint8_t AppDataSize = 0;            // Tamaño real del dato

// Contadores y flags
static uint32_t packagesSent = 0;              // Contador de paquetes enviados
static bool connEstablished = false;     // ¿Ya nos unimos al gateway?
static bool ReadyToSend = false;             // ¿Podemos enviar datos?

// Estructuras de callbacks (LoRaWAN avisa eventos por estas funciones)
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;

/*==============================================================================
 * PROTOTIPOS DE FUNCIONES
 *============================================================================*/
static void eventReceived(LoRaMacEventFlags_t *flags, LoRaMacEventInfo_t *info);
static void PrepareDataToSend(void);

/*==============================================================================
 * FUNCIÓN PRINCIPAL
 *============================================================================*/
int main(void)
{
    //==========================================================================
    // PASO 1: Inicializar la EDU-CIAA
    //==========================================================================
    boardConfig();                          // Inicializa clock, LEDs, etc.
    spiConfig(SPI0);                        // Configura SPI0 para hablar con SX1262
    uartConfig(UART_USB, 115200);           // UART para debug por USB
    
    printf("\r\n");
    printf("========================================\r\n");
    printf(" LoRaWAN - EDU-CIAA + SX1262\r\n");
    printf(" Gateway: RAK7268\r\n");
    printf(" Frecuencia: 915 MHz (AU915)\r\n");
    printf("========================================\r\n");

    //==========================================================================
    // PASO 2: Configurar los pines GPIO para el SX1262
    //==========================================================================
    printf("Configurando pines GPIO...\r\n");
    
    gpioConfig(RADIO_NSS, GPIO_OUTPUT);     // NSS = Chip Select (salida)
    gpioConfig(RADIO_BUSY, GPIO_INPUT);     // BUSY = señal de ocupado (entrada)
    gpioConfig(RADIO_NRESET, GPIO_OUTPUT);  // RESET = reinicio del chip (salida)
    gpioConfig(RADIO_DIO_1, GPIO_INPUT);    // DIO1 = interrupciones (entrada)
    
    gpioWrite(RADIO_NSS, 1);                // NSS en alto por defecto (no seleccionado)

    //==========================================================================
    // PASO 3: Inicializar el radio SX1262
    //==========================================================================
    printf("Inicializando radio SX1262...\r\n");
    
    Radio.Init();                           // Llama a RadioInit() en radio_adapter.c
                                            // Esto hace reset del chip y lo configura

    //==========================================================================
    // PASO 4: Configurar callbacks de LoRaWAN
    //==========================================================================
    printf("Configurando stack LoRaWAN...\r\n");
    
    // Callback principal: LoRaWAN nos avisará de eventos llamando a eventReceived()
    LoRaMacPrimitives.MacEvent = eventReceived;
    
    // Callbacks para obtener credenciales (usamos memcpy como truco simple)
    LoRaMacCallbacks.GetDevEui = (void (*)(uint8_t*))memcpy;
    LoRaMacCallbacks.GetJoinEui = (void (*)(uint8_t*))memcpy;
    LoRaMacCallbacks.GetAppKey = (void (*)(uint8_t*))memcpy;

    //==========================================================================
    // PASO 5: Inicializar el stack LoRaWAN
    //==========================================================================
    // Región AU915
    LoRaMacInitialization(&LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915);
    
    //==========================================================================
    // PASO 6: Configurar parámetros LoRaWAN (credenciales y opciones)
    //==========================================================================
    MibRequestConfirm_t mibReq;
    
    // Configurar DevEUI
    mibReq.Type = MIB_DEV_EUI;
    mibReq.Param.DevEui = (uint8_t*)LoRaWAN_DevEUI;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    // Configurar JoinEUI (AppEUI)
    mibReq.Type = MIB_JOIN_EUI;
    mibReq.Param.JoinEui = (uint8_t*)LoRaWAN_JoinEUI;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    // Configurar AppKey
    mibReq.Type = MIB_APP_KEY;
    mibReq.Param.AppKey = (uint8_t*)LoRaWAN_AppKey;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    // Habilitar red pública (necesario para gateways como RAK7268)
    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = true;
    LoRaMacMibSetRequestConfirm(&mibReq);
    
    // Habilitar ADR (Adaptive Data Rate) - el gateway ajusta la velocidad
    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = true;
    LoRaMacMibSetRequestConfirm(&mibReq);

    //==========================================================================
    // PASO 7: Iniciar proceso de JOIN (OTAA - Over The Air Activation)
    //==========================================================================
    printf("\r\nIniciando JOIN al gateway...\r\n");
    printf("Esperando respuesta...\r\n");
    
    LoRaMacJoin(NULL);                      // Envía solicitud de JOIN al gateway
                                            // El resultado llegará por eventReceived()

    //==========================================================================
    // PASO 8: BUCLE PRINCIPAL
    //==========================================================================
    while(true)
    {
        // --- TAREA A: Procesar la máquina de estados de LoRaWAN ---
        // Esto maneja: reintentos de JOIN, ventanas RX, timers, etc.
        LoRaMacProcess();
        
        // --- TAREA B: Procesar interrupciones del radio ---
        // Revisa el pin DIO1 y ejecuta callbacks cuando hay eventos
        Radio.IrqProcess();
        
        // --- TAREA C: Enviar datos si estamos listos ---
        if(connEstablished && ReadyToSend)
        {
            // Verificar que el stack no esté ocupado
            if(LoRaMacIsBusy() == false)
            {
                // Preparar datos del sensor (por ahora un mensaje de prueba)
                PrepareDataToSend();
                
                printf("\r\n[TX #%d] Enviando %d bytes...\r\n", packagesSent, AppDataSize);
                
                // Preparar frame LoRaWAN
                LoRaMacSendFrame_t frame;
                frame.Port = puertoLoraWAN;      // Puerto 15
                frame.NbBytes = AppDataSize;        // Tamaño del payload
                frame.Buffer = AppDataBuffer;       // Puntero a los datos
                frame.IsTxConfirmed = false;        // Sin ACK (unconfirmed uplink)
                
                // Enviar el frame
                LoRaMacSend(&frame, NULL);
                
                packagesSent++;
                ReadyToSend = false;
                
                // Esperar intervalo antes del próximo envío
                delay(intervalo);
                ReadyToSend = true;
            }
        }
    }

    return 0; // Nunca llega aquí
}

/*==============================================================================
 Prepara los datos a enviar (sensores)
 *============================================================================*/
static void PrepareDataToSend(void)
{
    // Por ahora enviamos un mensaje de prueba
    // DESPUÉS vas a reemplazar esto con los datos reales de tus sensores
    
    char mensaje[64];
    sprintf(mensaje, "Hola Gateway! Pkt#%d", packagesSent);
    
    AppDataSize = strlen(mensaje);
    memcpy(AppDataBuffer, mensaje, AppDataSize);
    
    printf("Datos: %s\r\n", mensaje);
    
    // EJEMPLO de cómo enviarás datos de sensores después:
    /*
    float temperatura = leerSensorTemperatura();
    float humedad = leerSensorHumedad();
    uint16_t luz = leerSensorLuz();
    
    AppDataBuffer[0] = (uint8_t)(temperatura * 10); // Temp x10 (ej: 25.3°C -> 253)
    AppDataBuffer[1] = (uint8_t)(humedad * 2);      // Humedad x2
    AppDataBuffer[2] = (luz >> 8) & 0xFF;           // Luz byte alto
    AppDataBuffer[3] = luz & 0xFF;                  // Luz byte bajo
    AppDataSize = 4;
    */
}

/*==============================================================================
 * FUNCIÓN: eventReceived (CALLBACK)
 * DESCRIPCIÓN: Llamada por el stack LoRaWAN cuando ocurre un evento
 *============================================================================*/
static void eventReceived(LoRaMacEventFlags_t *flags, LoRaMacEventInfo_t *info)
{
    //==========================================================================
    // EVENTO 1: JOIN aceptado por el gateway
    //==========================================================================
    if(flags->Bits.JoinAccept == 1)
    {
        if(info->Status == LORAMAC_STATUS_OK)
        {
            printf("\r\n");
            printf("========================================\r\n");
            printf(" ✓ JOIN EXITOSO!\r\n");
            printf(" Dispositivo conectado al gateway\r\n");
            printf("========================================\r\n");
            
            connEstablished = true;     // Ya estamos en la red
            ReadyToSend = true;         // Podemos empezar a enviar datos
        }
        else
        {
            printf("\r\n✗ JOIN FALLIDO (Status: %d)\r\n", info->Status);
            printf("Reintentando...\r\n");
            connEstablished = false;
            // El stack reintentará automáticamente
        }
    }
    
    //==========================================================================
    // EVENTO 2: Transmisión completada
    //==========================================================================
    if(flags->Bits.TxDone == 1)
    {
        printf("[TX] Transmisión completada\r\n");
    }
    
    //==========================================================================
    // EVENTO 3: Datos recibidos (downlink del gateway)
    //==========================================================================
    if(flags->Bits.RxDone == 1)
    {
        printf("\r\n[RX] Datos recibidos del gateway:\r\n");
        printf("     Tamaño: %d bytes\r\n", info->RxBufferSize);
        printf("     RSSI: %d dBm\r\n", info->RxRssi);
        printf("     SNR: %d dB\r\n", info->RxSnr);
        
        // Si el gateway te envía comandos, los procesarías aquí
        if(info->RxBufferSize > 0)
        {
            printf("     Payload: ");
            for(uint8_t i = 0; i < info->RxBufferSize; i++)
            {
                printf("%02X ", info->RxBuffer[i]);
            }
            printf("\r\n");
        }
    }
    
    //==========================================================================
    // EVENTO 4: Error en transmisión
    //==========================================================================
    if(flags->Bits.TxTimeout == 1)
    {
        printf("[TX] ✗ Timeout en transmisión\r\n");
        ReadyToSend = true; // Reintentar
    }
}
