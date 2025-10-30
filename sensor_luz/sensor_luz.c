
/*=====[Inclusions of function dependencies]=================================*/

//#include "sensor_luz.h"
//#include "sapi.h"

/*=====[Definition macros of private constants]==============================*/

/*=====[Definitions of extern global variables]==============================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Main function, program entry point after power on or reset]==========*/


// --- Inclusiones de SAPI ---
#include "sapi.h"
#include "board.h"

int main( void )
{
   // --- Inicialización de la placa ---
   boardConfig();

   // --- Inicialización de la UART_USB a 115200 baudios ---
   // La usaremos para enviar los datos a la computadora y verlos en una terminal.
   uartConfig( UART_USB, 115200 );

   // --- Inicialización del Conversor Analógico a Digital (ADC) ---
   // Es necesario habilitarlo para poder usarlo.
   adcConfig( ADC_ENABLE );

   // Variable para almacenar el valor leído del ADC
   uint16_t valorADC;
   uint8_t porcentajeLuz; // valor de luz en porcentaje (0 a 100)
   
   printf( "Sensor de luz LM393 (lectura analogica) inicializado.\r\n" );
   printf( "Cubra y descubra el sensor para ver como cambian los valores.\r\n\n" );

   // --- Bucle principal (se ejecuta permanentemente) ---
   while( true ) {

      // Leemos el canal analógico 1 (ADC_CH1), donde conectamos el sensor.
      // La función devuelve un valor entre 0 y 1023 (para una resolución de 10 bits).
      valorADC = adcRead( CH1 );
      porcentajeLuz = (uint8_t)((1-((float)valorADC / 1023.0f) )* 100.0f);
      // Imprimimos el valor leído por la UART_USB.
      // El \r\n es para que la terminal baje de línea en cada nueva lectura.
      printf( "Valor del sensor de luz (ADC): %d\r\n", valorADC );
      printf( "Valor en porcentaje sensor de luz: %d\r\n", porcentajeLuz);

      // Esperamos para la siguiente lectura.
      delay( 2000 );
   }

   return 0;
}
