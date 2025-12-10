// Archivo: src/sx126x-hal.c (VERSIÓN CORREGIDA 2)

#include "sapi.h"
#include "sx126x_hal.h"

// Mapeo de pines a los GPIO de SAPI
#define RADIO_NSS       GPIO0
#define RADIO_BUSY      GPIO1
#define RADIO_NRESET    GPIO2
#define RADIO_DIO_1     GPIO3

sx126x_hal_status_t sx126x_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    ( void )context;
    gpioWrite( RADIO_NSS, 0 ); // CORREGIDO: Usar 0 para BAJO
    spiWrite( SPI0, ( uint8_t* )command, command_length );
    spiWrite( SPI0, ( uint8_t* )data, data_length );
    gpioWrite( RADIO_NSS, 1 ); // CORREGIDO: Usar 1 para ALTO
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    ( void )context;
    gpioWrite( RADIO_NSS, 0 ); // CORREGIDO: Usar 0 para BAJO
    spiWrite( SPI0, ( uint8_t* )command, command_length );
    spiRead( SPI0, data, data_length );
    gpioWrite( RADIO_NSS, 1 ); // CORREGIDO: Usar 1 para ALTO
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset( const void* context )
{
    ( void )context;
    delay( 20 );
    gpioWrite( RADIO_NRESET, 0 ); // CORREGIDO: Usar 0 para BAJO
    delay( 50 );
    gpioWrite( RADIO_NRESET, 1 ); // CORREGIDO: Usar 1 para ALTO
    delay( 20 );
    return SX126X_HAL_STATUS_OK;
}

void sx126x_hal_wait_on_busy( void )
{
    while( gpioRead( RADIO_BUSY ) == 1 ); // CORREGIDO: Usar 1 para ALTO
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context )
{
    ( void )context;
    gpioWrite( RADIO_NSS, 0 ); // CORREGIDO: Usar 0 para BAJO
    gpioWrite( RADIO_NSS, 1 ); // CORREGIDO: Usar 1 para ALTO
    delay(2);
    return SX126X_HAL_STATUS_OK;
}