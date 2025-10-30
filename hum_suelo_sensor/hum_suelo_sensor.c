/*=============================================================================
 * Copyright (c) 2021, Fernando Pern�a <fpernia@fi.uba.ar>
 * Adaptado para sensor de humedad HL-69.
 * All rights reserved.
 * License: bsd-3-clause (see LICENSE.txt)
 * Date: 2021/11/03
 *===========================================================================*/

// --- Inclusiones de SAPI ---
#include "sapi.h"
#include "board.h"

int main( void )
{
   // --- Inicializaci�n de la placa ---
   boardConfig();

   // --- Inicializaci�n de la UART_USB a 115200 baudios ---
   uartConfig( UART_USB, 115200 );

   // --- Inicializaci�n del Conversor Anal�gico a Digital (ADC) ---
   adcConfig( ADC_ENABLE );

   // Variables para almacenar los valores
   uint16_t valorADC;
   uint8_t porcentajeHumedad; // Variable para el porcentaje de 0 a 100

   printf( "Sensor de humedad en suelo HL-69 inicializado.\r\n" );
   printf( "Pruebe con el sensor al aire (seco) y en agua (humedo).\r\n\n" );

   // --- Bucle principal (se ejecuta permanentemente) ---
   while( true ) {

      // 1. Leemos el canal anal�gico 1 (ADC_CH1). Rango: 0 a 1023.
      valorADC = adcRead( CH1 );

      // 2. Mapeamos el valor del ADC a un porcentaje de humedad.
      // Como un valor ADC ALTO significa SECO, debemos INVERTIR el resultado.
      // Un valor ADC cercano a 1023 ser� 0% de humedad.
      // Un valor ADC cercano a 0 ser� 100% de humedad.
      porcentajeHumedad = 100 - (((float)valorADC / 1023.0f) * 100.0f);

      // 3. Imprimimos el resultado en la consola.
      printf( "Humedad del suelo: %d %% (ADC: %d)\r\n", porcentajeHumedad, valorADC );

      // Esperamos 500 milisegundos antes de la siguiente lectura.
      delay( 2000 );
   }

   // No deber�a llegar nunca a esta parte
   return 0;
}
