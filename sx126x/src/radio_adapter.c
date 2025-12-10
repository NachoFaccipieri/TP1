/*=============================================================================
 * Archivo: radio_adapter.c
 * Descripción: Implementación del "contrato" de radio.h usando el driver sx126x.
 *
 * NOTA: Esta es una implementación simplificada para SAPI y EDU-CIAA.
 * Se enfoca en LoRa y omite FSK/GFSK.
 *===========================================================================*/

#include "sapi.h"       // Para 'delay'
#include "radio.h"      // El "contrato" que LoRaMac-node nos pide
#include "sx126x.h"     // Nuestro driver de chip, que sabe hablar con el hardware
#include "sx126x_hal.h" // Nuestro HAL, que sabe hablar con SAPI

/* --- Definiciones de pines (deben coincidir con sx126x-hal.c) --- */
#define RADIO_DIO_1     GPIO3
#define RADIO_BUSY      GPIO1

/* --- Constantes --- */
#define RF_FREQUENCY          915000000 // Frecuencia por defecto (será sobrescrita)
#define LORA_SYMBOL_TIMEOUT   5         // 5 símbolos

/* --- Variables Estáticas (privadas) --- */

// Puntero a los eventos de la pila LoRaMac
static RadioEvents_t *RadioEvents;

// El "context" de nuestro driver siempre es NULL
static const void* s_RadioContext = NULL;

// Estructuras para guardar la configuración de LoRa
static sx126x_mod_params_lora_t s_lora_mod_params;
static sx126x_pkt_params_lora_t s_lora_pkt_params;


/* --- Prototipos de Funciones Estáticas (las que implementan la interfaz) --- */

static void RadioInit( void );
static RadioState_t RadioGetStatus( void );
static void RadioSetModem( RadioModems_t modem );
static void RadioSetChannel( uint32_t freq );
static bool RadioIsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime );
static uint32_t RadioGetRandom( void );
static void RadioSetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                              uint32_t datarate, uint8_t coderate,
                              uint32_t bandwidthAfc, uint16_t preambleLen,
                              uint16_t symbTimeout, bool fixLen,
                              uint8_t payloadLen,
                              bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                              bool iqInverted, bool rxContinuous );
static void RadioSetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                              uint32_t bandwidth, uint32_t datarate,
                              uint8_t coderate, uint16_t preambleLen,
                              bool fixLen, bool crcOn, bool freqHopOn,
                              uint8_t hopPeriod, bool iqInverted, uint32_t timeout );
static void RadioTx( const uint8_t *payload, uint8_t size, uint32_t timeout );
static void RadioRx( uint32_t timeout );
static void RadioSleep( void );
static void RadioWrite( uint16_t addr, uint8_t data );
static uint8_t RadioRead( uint16_t addr );
static void RadioWriteBuffer( uint16_t addr, const uint8_t *buffer, uint8_t size );
static void RadioReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size );
static void RadioSetMaxPayloadLength( RadioModems_t modem, uint8_t max );
static void RadioSetPublicNetwork( bool enable );
static void RadioIrqProcess( void );


/* --- LA INTERFAZ PÚBLICA (EL "ADAPTADOR") --- */

// Esta es la estructura que "exportamos" a la pila LoRaMac.
const struct Radio_s Radio =
{
    .Init = RadioInit,
    .GetStatus = RadioGetStatus,
    .SetModem = RadioSetModem,
    .SetChannel = RadioSetChannel,
    .IsChannelFree = RadioIsChannelFree,
    .GetRandom = RadioGetRandom,
    .SetRxConfig = RadioSetRxConfig,
    .SetTxConfig = RadioSetTxConfig,
    .Tx = RadioTx,
    .Rx = RadioRx,
    .Sleep = RadioSleep,
    .Write = RadioWrite,
    .Read = RadioRead,
    .WriteBuffer = RadioWriteBuffer,
    .ReadBuffer = RadioReadBuffer,
    .SetMaxPayloadLength = RadioSetMaxPayloadLength,
    .SetPublicNetwork = RadioSetPublicNetwork,
    .IrqProcess = RadioIrqProcess,
    // Funciones que no usamos para LoRaWAN básico
    .GetTimeOnAir = NULL,
    .Send = NULL, // Usamos .Tx
    .SetRxDutyCycle = NULL,
    .SetCadParams = NULL,
    .SetAntSw = NULL,
    .CheckRfFrequency = NULL, // Asumimos que la frecuencia es válida
    .SetPayload = NULL,       // Deprecado
    .SetSyncWord = NULL,      // LoRaWAN usa un sync word estándar
    .SetTxContinuousWave = NULL,
    .SetRangingRole = NULL,
    .GetRangingResult = NULL,
};

/* --- Implementación de Funciones de la Interfaz --- */

static void RadioInit( void )
{
    // 1. Resetear el chip
    sx126x_hal_reset( s_RadioContext );

    // 2. Configurar el regulador (usar DCDC)
    sx126x_set_reg_mode( s_RadioContext, SX126X_REG_MODE_DCDC );
    
    // 3. Configurar pines DIO (Digital I/O)
    // Mapeamos las interrupciones que nos importan (TxDone, RxDone, Timeout)
    // al pin DIO1.
    uint16_t irqMask = SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT;
    uint16_t dio1Mask = irqMask; // Todo a DIO1
    uint16_t dio2Mask = SX126X_IRQ_NONE;
    uint16_t dio3Mask = SX126X_IRQ_NONE;
    
    sx126x_set_dio_irq_params( s_RadioContext, irqMask, dio1Mask, dio2Mask, dio3Mask );
    
    // 4. Limpiar cualquier IRQ pendiente
    sx126x_clear_irq_status( s_RadioContext, SX126X_IRQ_ALL );

    // 5. Configurar el modo de paquete (siempre LoRa para LoRaWAN)
    sx126x_set_pkt_type( s_RadioContext, SX126X_PKT_TYPE_LORA );
}

static RadioState_t RadioGetStatus( void )
{
    sx126x_chip_status_t status;
    sx126x_get_status( s_RadioContext, &status );

    // Traducir el estado del chip al estado genérico de Radio.h
    switch( status.chip_mode )
    {
        case SX126X_CHIP_MODE_STBY_RC:
        case SX126X_CHIP_MODE_STBY_XOSC:
            return RADIO_IDLE;
        case SX126X_CHIP_MODE_TX:
            return RADIO_TX_RUNNING;
        case SX126X_CHIP_MODE_RX:
            return RADIO_RX_RUNNING;
        case SX126X_CHIP_MODE_SLEEP:
            return RADIO_SLEEP;
        default:
            return RADIO_IDLE;
    }
}

static void RadioSetModem( RadioModems_t modem )
{
    // Solo soportamos LoRa
    if( modem == MODEM_LORA )
    {
        sx126x_set_pkt_type( s_RadioContext, SX126X_PKT_TYPE_LORA );
    }
}

static void RadioSetChannel( uint32_t freq )
{
    sx126x_set_rf_freq( s_RadioContext, freq );
}

static bool RadioIsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    // NOTA: Esta es una implementación simplificada.
    // Una implementación completa usaría el modo CAD (Channel Activity Detection)
    // Por ahora, asumimos que el canal siempre está libre.
    return true; 
}

static uint32_t RadioGetRandom( void )
{
    uint32_t random_val = 0;
    sx126x_get_random_numbers( s_RadioContext, &random_val, 1 );
    return random_val;
}

static void RadioSetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                              uint32_t datarate, uint8_t coderate,
                              uint32_t bandwidthAfc, uint16_t preambleLen,
                              uint16_t symbTimeout, bool fixLen,
                              uint8_t payloadLen,
                              bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                              bool iqInverted, bool rxContinuous )
{
    // 1. Traducir Spreading Factor (datarate)
    switch( datarate )
    {
        case 7: s_lora_mod_params.sf = SX126X_LORA_SF7; break;
        case 8: s_lora_mod_params.sf = SX126X_LORA_SF8; break;
        case 9: s_lora_mod_params.sf = SX126X_LORA_SF9; break;
        case 10: s_lora_mod_params.sf = SX126X_LORA_SF10; break;
        case 11: s_lora_mod_params.sf = SX126X_LORA_SF11; break;
        case 12: s_lora_mod_params.sf = SX126X_LORA_SF12; break;
        default: s_lora_mod_params.sf = SX126X_LORA_SF7; break;
    }

    // 2. Traducir Bandwidth
    switch( bandwidth )
    {
        case 0: s_lora_mod_params.bw = SX126X_LORA_BW_125; break;
        case 1: s_lora_mod_params.bw = SX126X_LORA_BW_250; break;
        case 2: s_lora_mod_params.bw = SX126X_LORA_BW_500; break;
        default: s_lora_mod_params.bw = SX126X_LORA_BW_125; break;
    }
    
    // 3. Traducir Coding Rate
    switch( coderate )
    {
        case 1: s_lora_mod_params.cr = SX126X_LORA_CR_4_5; break;
        case 2: s_lora_mod_params.cr = SX126X_LORA_CR_4_6; break;
        case 3: s_lora_mod_params.cr = SX126X_LORA_CR_4_7; break;
        case 4: s_lora_mod_params.cr = SX126X_LORA_CR_4_8; break;
        default: s_lora_mod_params.cr = SX126X_LORA_CR_4_5; break;
    }

    // 4. Configurar Low Data Rate Optimize (LDRO)
    // Se activa automáticamente para SF11/SF12 con BW125 (o SF > 10)
    if( ( ( s_lora_mod_params.sf == SX126X_LORA_SF11 ) || ( s_lora_mod_params.sf == SX126X_LORA_SF12 ) ) && ( s_lora_mod_params.bw == SX126X_LORA_BW_125 ) )
    {
        s_lora_mod_params.ldro = 1; // Activar
    }
    else
    {
        s_lora_mod_params.ldro = 0; // Desactivar
    }
    
    // Aplicar la configuración de modulación
    sx126x_set_lora_mod_params( s_RadioContext, &s_lora_mod_params );


    // 5. Configurar parámetros del paquete
    s_lora_pkt_params.preamble_len_in_symb = preambleLen;
    s_lora_pkt_params.header_type = fixLen ? SX126X_LORA_PKT_IMPLICIT : SX126X_LORA_PKT_EXPLICIT;
    s_lora_pkt_params.pld_len_in_bytes = payloadLen;
    s_lora_pkt_params.crc_is_on = crcOn;
    s_lora_pkt_params.invert_iq_is_on = iqInverted;

    sx126x_set_lora_pkt_params( s_RadioContext, &s_lora_pkt_params );
    
    // 6. Configurar el timeout de símbolo
    // LoRaMac-node usa symbTimeout = 0 para RX "Single Shot" (no continuo)
    // y symbTimeout > 0 para RX continuo.
    if( rxContinuous == true )
    {
        sx126x_set_lora_symb_nb_timeout( s_RadioContext, symbTimeout );
    }
    else
    {
        // Para RX Single, LoRaMac-node espera que el timeout se pase
        // directamente en la función RadioRx().
        // Seteamos el timeout de símbolo a 0 para desactivarlo aquí.
        sx126x_set_lora_symb_nb_timeout( s_RadioContext, 0 );
    }
}

static void RadioSetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                              uint32_t bandwidth, uint32_t datarate,
                              uint8_t coderate, uint16_t preambleLen,
                              bool fixLen, bool crcOn, bool freqHopOn,
                              uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    // 1. Configurar los parámetros de modulación (SF, BW, CR)
    // Reutilizamos la misma lógica que en SetRxConfig
    // (datarate = SF, bandwidth = 0/1/2, coderate = 1/2/3/4)
    
    RadioSetRxConfig( modem, bandwidth, datarate, coderate, 0,
                      preambleLen, 0, fixLen, 0,
                      crcOn, freqHopOn, hopPeriod, iqInverted, false );

    // 2. Configurar la potencia de salida
    // TODO: Mapear 'power' (que es un índice genérico) a dBm reales.
    // Por ahora, usamos 14 dBm fijos como en tu main.c viejo.
    sx126x_set_tx_params( s_RadioContext, 14, SX126X_RAMP_200_US );
}

static void RadioTx( const uint8_t *payload, uint8_t size, uint32_t timeout )
{
    // 1. Poner el payload en el buffer del chip
    // Usamos offset 0.
    // También actualizamos el largo del payload en los parámetros del paquete.
    s_lora_pkt_params.pld_len_in_bytes = size;
    sx126x_set_lora_pkt_params( s_RadioContext, &s_lora_pkt_params );
    sx126x_write_buffer( s_RadioContext, 0, payload, size );

    // 2. Poner el chip en modo TX
    // El timeout de 0 significa "sin timeout" (esperará a que termine solo)
    // LoRaMac-node maneja el timeout por software.
    sx126x_set_tx( s_RadioContext, 0 ); 
}

static void RadioRx( uint32_t timeout )
{
    // Poner el chip en modo RX
    // 'timeout' viene en milisegundos.
    // timeout = 0 significa "RX Continuo"
    // timeout > 0 significa "RX Single Shot" (se apaga después del timeout)
    
    if( timeout == 0 )
    {
        // RX Continuo
        sx126x_set_rx( s_RadioContext, SX126X_RX_CONTINUOUS );
    }
    else
    {
        // RX Single
        sx126x_set_rx( s_RadioContext, timeout );
    }
}

static void RadioSleep( void )
{
    sx126x_set_sleep( s_RadioContext, SX126X_SLEEP_CFG_WARM_START );
}

static void RadioWrite( uint16_t addr, uint8_t data )
{
    sx126x_write_register( s_RadioContext, addr, &data, 1 );
}

static uint8_t RadioRead( uint16_t addr )
{
    uint8_t data;
    sx126x_read_register( s_RadioContext, addr, &data, 1 );
    return data;
}

static void RadioWriteBuffer( uint16_t addr, const uint8_t *buffer, uint8_t size )
{
    // La función WriteBuffer del driver asume el registro 0x0E (comando)
    // y solo toma el offset (que LoRaMac-node pone en 'addr')
    sx126x_write_buffer( s_RadioContext, (uint8_t)addr, buffer, size );
}

static void RadioReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    // Idem WriteBuffer
    sx126x_read_buffer( s_RadioContext, (uint8_t)addr, buffer, size );
}

static void RadioSetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    // La pila llama a esto para decirnos el tamaño máximo de paquete.
    // Lo guardamos en nuestra configuración de paquete.
    s_lora_pkt_params.pld_len_in_bytes = max;
    sx126x_set_lora_pkt_params( s_RadioContext, &s_lora_pkt_params );
}

static void RadioSetPublicNetwork( bool enable )
{
    // LoRaWAN usa un "Sync Word" (palabra de sincronización)
    // 0x34 para redes públicas (como The Things Network)
    // 0x12 para redes privadas
    
    if( enable )
    {
        sx126x_set_lora_sync_word( s_RadioContext, 0x34 );
    }
    else
    {
        sx126x_set_lora_sync_word( s_RadioContext, 0x12 );
    }
}


/**
 * @brief Esta es la función CLAVE de manejo de interrupciones.
 * Tu main.c deberá llamar a esta función en bucle
 * o (idealmente) cuando detecte que el pin DIO1 (GPIO3) se puso en ALTO.
 */
static void RadioIrqProcess( void )
{
    // 1. Revisar si el pin DIO1 (que es nuestra señal de IRQ) está en ALTO.
    // Usamos gpioRead() de SAPI.
    
    if( gpioRead( RADIO_DIO_1 ) == 1 )
    {
        sx126x_irq_mask_t irq_flags;

        // 2. Obtener y Limpiar las banderas de interrupción del chip
        sx126x_get_and_clear_irq_status( s_RadioContext, &irq_flags );

        // 3. Revisar qué interrupción ocurrió y notificar a la pila LoRaMac
        
        // --- Transmisión Completa ---
        if( ( irq_flags & SX126X_IRQ_TX_DONE ) == SX126X_IRQ_TX_DONE )
        {
            // Poner el chip en Standby para ahorrar energía
            sx126x_set_standby( s_RadioContext, SX126X_STANDBY_CFG_RC );
            
            // Notificar a la pila que la TX terminó
            if( RadioEvents != NULL && RadioEvents->TxDone != NULL )
            {
                RadioEvents->TxDone();
            }
        }

        // --- Recepción Completa ---
        if( ( irq_flags & SX126X_IRQ_RX_DONE ) == SX126X_IRQ_RX_DONE )
        {
            uint8_t rx_payload[255];
            uint8_t rx_size = 0;
            sx126x_pkt_status_lora_t pkt_status;
            sx126x_rx_buffer_status_t rx_status;

            // Poner el chip en Standby
            sx126x_set_standby( s_RadioContext, SX126X_STANDBY_CFG_RC );

            // Obtener información del paquete (RSSI, SNR)
            sx126x_get_lora_pkt_status( s_RadioContext, &pkt_status );
            
            // Obtener información del buffer (cuántos bytes llegaron y dónde)
            sx126x_get_rx_buffer_status( s_RadioContext, &rx_status );

            rx_size = rx_status.pld_len_in_bytes;

            // Leer el payload desde el buffer del chip
            sx126x_read_buffer( s_RadioContext, rx_status.buffer_start_pointer, rx_payload, rx_size );

            // Notificar a la pila que llegó un paquete
            if( RadioEvents != NULL && RadioEvents->RxDone != NULL )
            {
                RadioEvents->RxDone( rx_payload, rx_size, pkt_status.rssi_pkt_in_dbm, pkt_status.snr_pkt_in_db );
            }
        }
        
        // --- Timeout (de TX o RX) ---
        if( ( irq_flags & SX126X_IRQ_TIMEOUT ) == SX126X_IRQ_TIMEOUT )
        {
            // Poner el chip en Standby
            sx126x_set_standby( s_RadioContext, SX126X_STANDBY_CFG_RC );

            // Notificar a la pila que hubo un Timeout
            if( RadioEvents != NULL && RadioEvents->TxTimeout != NULL && RadioEvents->RxTimeout != NULL )
            {
                // No sabemos si el timeout fue de TX o RX, así que notificamos ambos
                // (la pila sabrá cuál estaba esperando)
                RadioEvents->TxTimeout();
                RadioEvents->RxTimeout();
            }
        }
    }
}