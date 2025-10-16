/*=====[Inclusions of function dependencies]=================================*/

#include "program_main.h"
//#include "leer_adc.h"

/*=====[Definition macros of private constants]==============================*/

/*=====[Definitions of extern global variables]==============================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Main function, program entry point after power on or reset]==========*/

int main( void )
{
   // ----- Setup -----------------------------------
   // --- Inicialización de la placa ---
   boardConfig();

   // --- Inicialización de la UART_USB a 115200 baudios ---
   // La usaremos para enviar los datos a la computadora y verlos en una terminal.
   uartConfig( UART_USB, 115200 );

   // --- Inicialización del Conversor Analógico a Digital (ADC) ---
   adcConfig( ADC_ENABLE );

   // Variable para almacenar el valor leído del ADC
   uint8_t porcentajeLuz; // valor de luz en porcentaje (0 a 100)
   uint8_t porcentajeHumSuelo; // Variable para el porcentaje de 0 a 100

   // ----- Repeat for ever -------------------------
   while( true ) {
      
      //A la funcion leerADC se le debe pasar el valor del canal por el cual hacemos la lectura.
      porcentajeLuz = leerADC(1);
      porcentajeHumSuelo = leerADC(2);
 
      // El \r\n es para que la terminal baje de línea en cada nueva lectura.
      printf( "Valor en porcentaje sensor de humedad del suelo: %d\r\n", porcentajeHumSuelo);
      printf( "Valor en porcentaje sensor de luz: %d\r\n", porcentajeLuz);

      // Esperamos para la siguiente lectura.
      delay( 8000 );
   }

   return 0;
}
