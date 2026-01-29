/*=====[Inclusions of function dependencies]=================================*/

#include "program_main.h"
#include "sapi_dht11.h"

/*=====[Definition macros of private constants]==============================*/
#define DHT11_PIN   GPIO0 /* pin GPIO donde conectar el DHT11*/

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
   
   // --- Inicialización del sensor DHT11 ---
   dht11Init( DHT11_PIN );

   // Variable para almacenar el valor leído del ADC
   uint8_t porcentajeLuz; // valor de luz en porcentaje (0 a 100)
   uint8_t porcentajeHumSuelo; // Variable para el porcentaje de 0 a 100

   // Variables para almacenar los valores del DHT11
   float temperatura; 
   float humedad_aire;

   // ----- Repeat for ever -------------------------
   while( true ) {
      
      //A la funcion leerADC se le debe pasar el valor del canal por el cual hacemos la lectura.
      porcentajeLuz = leerADC(3);
      porcentajeHumSuelo = leerADC(2);
 
      // El \r\n es para que la terminal baje de línea en cada nueva lectura.
      printf( "Valor en porcentaje sensor de humedad del suelo: %d\r\n", porcentajeHumSuelo);
      printf( "Valor en porcentaje sensor de luz: %d\r\n", porcentajeLuz);
      
      if( dht11Read( &humedad_aire, &temperatura ) ) { // <-- AÑADIDO
         
         // Impresión de temperatura y humedad
         printf( "Temperatura: %d C\r\n", temperatura);
         printf( "Humedad del Aire: %d %%\r\n", humedad_aire );

      } else {
         // Si la lectura falla (común en el DHT11 por timeouts o error de CRC)
         printf( "Error al leer el sensor DHT11.\r\n" ); // <-- AÑADIDO
      }

      // Esperamos para la siguiente lectura. //minimo de 2seg para dht11
      delay( 3000 );
   }

   return 0;
}
