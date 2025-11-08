/*=============================================================================
 * Archivo: main.c
 * Descripción: Ejemplo de transmisor LoRaWAN (OTAA) con SX1262 y EDU-CIAA (SAPI).
 * Utiliza la pila LoRaMac-node.
 *===========================================================================*/

#include "sapi.h"
#include "radio.h"          // La interfaz de radio que implementamos
#include "LoRaMac.h"        // La pila LoRaWAN (nuestro "cerebro")
#include "region/Region.h"  // Necesario para configurar la región (ej. AU915)

/* --- Configuración de Pines --- */
// Deben coincidir con los pines en sx126x-hal.c y radio_adapter.c
#define RADIO_NSS       GPIO0
#define RADIO_BUSY      GPIO1
#define RADIO_NRESET    GPIO2
#define RADIO_DIO_1     GPIO3 // ¡El pin de interrupción!

/*=============================================================================
 * ¡¡¡ CREDENCIALES !!!
 *===========================================================================*/
// Estas son las credenciales que te da tu servidor de red (ej. The Things Network)

// 1. DevEUI (Device EUI) - 8 bytes, LSB (Least Significant Byte first)
// Tu EUI (MSB): 1111000011110000
// Escrito en LSB (al revés, byte a byte):
static const uint8_t LoRaWAN_DevEUI[8] = { 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x11, 0x11 };

// 2. JoinEUI (o AppEUI) - 8 bytes, LSB
// No se especifica en la UI de RAK, usamos el default.
// Valor por defecto (MSB): 0000000000000000
// Escrito en LSB (al revés, queda igual):
static const uint8_t LoRaWAN_JoinEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// 3. AppKey (Application Key) - 16 bytes, MSB (Most Significant Byte first)
// Esta clave se escribe en el orden normal (MSB).
// Tu clave (MSB): 11110000111100001111000011110000
static const uint8_t LoRaWAN_AppKey[16] = {
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00,
    0x11, 0x11, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00
};
/*=============================================================================
 * Configuración del Paquete a Enviar
 *===========================================================================*/

// Puerto de la aplicación LoRaWAN (ej. 15)
#define LORAWAN_APP_PORT                15

// Datos a enviar (payload)
static uint8_t AppDataBuffer[] = "Hola EDU-CIAA!";
static uint32_t TxCounter = 0;

// Estado del dispositivo (para saber si ya nos unimos a la red)
static bool IsNetworkJoined = false;
static bool IsTxPending = false; // Flag para saber si queremos enviar un paquete

/* --- Prototipos de Callbacks de LoRaMac --- */
// La pila LoRaMac nos "avisará" cuando ocurran eventos
// llamando a estas funciones (que nosotros le pasamos).

static void OnMacEvent( LoRaMacEventFlags_t *flags, LoRaMacEventInfo_t *info );
static void OnJoinRequest( LoRaMacStatus_t status );
static void OnTxDone( void );
static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

// Estructura que contiene los punteros a nuestras funciones de callback
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;

/* --- Función Principal --- */

int main( void )
{
    // 1. Inicialización de la placa SAPI
    boardConfig();
    spiConfig( SPI0 );
    uartConfig( UART_USB, 115200 );
    printf("--- Ejemplo LoRaWAN (OTAA) EDU-CIAA ---\r\n");

    // 2. Configuración de los pines para el SX1262
    gpioConfig(RADIO_NSS, GPIO_OUTPUT);
    gpioConfig(RADIO_BUSY, GPIO_INPUT);
    gpioConfig(RADIO_NRESET, GPIO_OUTPUT);
    gpioConfig(RADIO_DIO_1, GPIO_INPUT); // Pin de interrupción
    
    gpioWrite(RADIO_NSS, 1); // NSS en alto por defecto

    // 3. Inicializar la interfaz de Radio (llama a RadioInit() en radio_adapter.c)
    Radio.Init();

    // 4. Inicializar la pila LoRaMac
    LoRaMacPrimitives.MacEvent = OnMacEvent;
    LoRaMacCallbacks.GetDevEui = ( void (*)( uint8_t* ) )memcpy; // Truco para pasar el puntero
    LoRaMacCallbacks.GetJoinEui = ( void (*)( uint8_t* ) )memcpy;
    LoRaMacCallbacks.GetAppKey = ( void (*)( uint8_t* ) )memcpy;
    
    LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915 );
    
    // 5. Configurar los parámetros para el Join (Unión)
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_DEV_EUI;
    mibReq.Param.DevEui = (uint8_t*)LoRaWAN_DevEUI;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_JOIN_EUI;
    mibReq.Param.JoinEui = (uint8_t*)LoRaWAN_JoinEUI;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_APP_KEY;
    mibReq.Param.AppKey = (uint8_t*)LoRaWAN_AppKey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    // Habilitar modo público (para The Things Network, etc.)
    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = true;
    LoRaMacMibSetRequestConfirm( &mibReq );

    // Habilitar Adaptive Data Rate (ADR)
    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = true;
    LoRaMacMibSetRequestConfirm( &mibReq );

    // 6. ¡Enviar la solicitud de Join (OTAA)!
    printf("Iniciando Join (OTAA) a la red...\r\n");
    LoRaMacJoin( NULL ); // LoRaMacJoin( ) fue reemplazado por LoRaMacSendJoinRequest( ) en v5+
                         // Si usas una pila más nueva, esta función puede cambiar.
                         // Asumimos LoRaMacJoin por ahora.

    // 7. Bucle Principal
    while( true )
    {
        // === TAREA A ===
        // Procesar la máquina de estados de LoRaMac
        // (Maneja los tiempos, reintentos de Join, ventanas de RX, etc.)
        LoRaMacProcess( );

        // === TAREA B ===
        // Procesar las interrupciones de Radio
        // (Revisa el pin DIO1 y llama a los callbacks TxDone/RxDone)
        Radio.IrqProcess( );

        // === TAREA C ===
        // Enviar un paquete (si ya nos unimos y no estamos ocupados)
        if( IsNetworkJoined == true && IsTxPending == true )
        {
            if( LoRaMacIsBusy( ) == false )
            {
                printf("Enviando paquete %d: %s\r\n", TxCounter, AppDataBuffer);

                // Enviar el paquete
                LoRaMacSendFrame_t frame;
                frame.Port = LORAWAN_APP_PORT;
                frame.NbBytes = sizeof(AppDataBuffer) - 1; // No enviar el '\0'
                frame.Buffer = AppDataBuffer;
                frame.IsTxConfirmed = false; // No pedir confirmación (ACK)

                LoRaMacSend( &frame, NULL ); // LoRaMacSend( ) fue reemplazado por LoRaMacSendFrame( ) en v5+

                TxCounter++;
                IsTxPending = false;
                
                delay(20000); // Esperar 20 segundos para el próximo envío
                IsTxPending = true; // Marcar para el próximo envío
            }
        }
    }

    return 0; // No debería llegar aquí
}


/* --- Implementación de Callbacks --- */

/**
 * @brief Llamado por la pila LoRaMac cuando la solicitud de Join termina.
 */
static void OnJoinRequest( LoRaMacStatus_t status )
{
    if( status == LORAMAC_STATUS_OK )
    {
        printf("¡Join exitoso! Dispositivo unido a la red.\r\n");
        IsNetworkJoined = true;
        IsTxPending = true; // Marcar para enviar el primer paquete
    }
    else
    {
        printf("Join fallido. Reintentando...\r\n");
        IsNetworkJoined = false;
        // La pila LoRaMac reintentará automáticamente
    }
}

/**
 * @brief Llamado por la pila LoRaMac cuando la transmisión (TX) se completa.
 */
static void OnTxDone( void )
{
    printf("TX completado.\r\n");
}

/**
 * @brief Llamado por la pila LoRaMac cuando se recibe un paquete (RX).
 */
static void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    printf("RX: Paquete recibido. RSSI: %d, SNR: %d, Size: %d\r\n", rssi, snr, size);
    // (Aquí podrías procesar un "downlink" si tu app lo envía)
}

/**
 * @brief Función de callback principal de la pila.
 */
static void OnMacEvent( LoRaMacEventFlags_t *flags, LoRaMacEventInfo_t *info )
{
    if( flags->Bits.JoinAccept == 1 )
    {
        OnJoinRequest( info->Status );
    }

    if( flags->Bits.TxDone == 1 )
    {
        OnTxDone();
    }

    if( flags->Bits.RxDone == 1 )
    {
        OnRxDone( info->RxBuffer, info->RxBufferSize, info->RxRssi, info->RxSnr );
    }
}