/*=====[Inclusions of function dependencies]=================================*/
#include "program_main.h"
#include "sapi.h"         // Incluye RTC, GPIO, UART, etc.
#include "sapi_dht11.h"

/*=====[Definition macros of private constants]==============================*/
#define DHT11_PIN       GPIO0        // Pin del sensor DHT11
#define ESP32_WAKE_PIN  GPIO1        // Pin conectado al GPIO33 del ESP32
#define UART_ESP32      UART_232     // UART conectada al ESP32
#define TIEMPO_SLEEP    60           // Tiempo a dormir en SEGUNDOS

/*=====[Private function prototypes]=========================================*/
void rtc_sumar_segundos( rtc_t *rtc_actual, int segundos_a_sumar, rtc_t *rtc_destino );
void dormir_sistema_hasta_alarma( uint32_t segundos );

/*=====[Main function]=======================================================*/

int main( void )
{
   // --- 1. Inicialización ---
   boardConfig();
   uartConfig( UART_USB, 115200 );    // Para debug en PC
   uartConfig( UART_ESP32, 9600 );    // Para hablar con ESP32

   adcConfig( ADC_ENABLE );
   dht11Init( DHT11_PIN );
   
   // Configurar Pin de "Despertador"
   gpioConfig( ESP32_WAKE_PIN, GPIO_OUTPUT );
   gpioWrite( ESP32_WAKE_PIN, 0 );    // Mantener bajo por defecto

   // Configurar e Inicializar RTC
   rtcConfig( RTC_ENABLE );
   rtc_t rtc_now;
   // Seteamos una fecha base arbitraria para empezar a contar
   rtc_now.year = 2026; rtc_now.month = 1; rtc_now.mday = 29;
   rtc_now.hour = 12;   rtc_now.min = 0;   rtc_now.sec = 0;
   rtcWrite( &rtc_now );

   // Variables de sensores
   uint8_t porcentajeLuz, porcentajeHumSuelo;
   float temperatura = 0, humedad_aire = 0;
   char buffer_tx[64];

   // --- 2. Bucle Principal ---
   while( true ) {
      
      // A. Leer Sensores
      porcentajeLuz = leerADC(3); // Asumiendo canal 3
      porcentajeHumSuelo = leerADC(2); // Asumiendo canal 2
      
      if( !dht11Read( &humedad_aire, &temperatura ) ) {
         // Si falla, enviamos valores de error o los últimos válidos
         printf("Error DHT11\r\n");
      }

      // B. Despertar al ESP32 (Handshake)
      // Generamos un pulso positivo de 100ms para disparar el wake-up del ESP32
      printf("Despertando ESP32...\r\n");
      gpioWrite( ESP32_WAKE_PIN, 1 ); 
      delay( 200 ); // Tiempo suficiente para que el hardware detecte el flanco
      gpioWrite( ESP32_WAKE_PIN, 0 ); 
      
      // Esperamos un momento a que el ESP32 arranque y configure su UART
      delay( 1000 ); 

      // C. Enviar datos
      // Formato CSV simple: H_Suelo,Luz,Temp,H_Aire
      sprintf(buffer_tx, "S:%d,L:%d,T:%.0f,H:%.0f\r\n", 
              porcentajeHumSuelo, porcentajeLuz, temperatura, humedad_aire);
      
      printf("Enviando a ESP32: %s", buffer_tx); // Debug PC
      uartWriteString( UART_ESP32, buffer_tx );

      // Esperar a que salga la transmisión UART antes de dormirnos
      delay( 500 ); 

      // D. Dormir el Sistema
      printf("Durmiendo EDU-CIAA por %d segundos...\r\n", TIEMPO_SLEEP);
      
      // Esta función pone la CPU en modo bajo consumo y espera al RTC
      dormir_sistema_hasta_alarma( TIEMPO_SLEEP );
      
      printf("EDU-CIAA Despierta!\r\n");
   }

   return 0;
}

/*=====[Implementations of private functions]================================*/

// Función que maneja el modo Sleep esperando al RTC
void dormir_sistema_hasta_alarma( uint32_t segundos ) {
    rtc_t actual, alarma;
    
    // 1. Leer hora actual
    rtcRead( &actual );
    
    // 2. Calcular hora de despertar
    rtc_sumar_segundos( &actual, segundos, &alarma );
    
    // 3. Bucle de Sueño (Sleep Loop)
    // Mientras la hora actual sea MENOR a la hora de alarma, dormimos la CPU.
    // Usamos __WFI() (Wait For Interrupt). Esto apaga la CPU hasta la próxima
    // interrupción. Como la EDU-CIAA tiene el SysTick (para delays) corriendo,
    // se despertará cada 1ms, comprobará la hora, y se volverá a dormir si no es tiempo.
    // Esto ahorra mucha energía comparado con un delay() normal que mantiene la CPU al 100%.
    
    while( true ) {
        rtcRead( &actual );
        
        // Comprobar si llegamos al tiempo (comparación simple jerárquica)
        bool tiempo_cumplido = false;
        if (actual.year > alarma.year) tiempo_cumplido = true;
        else if (actual.year == alarma.year) {
            if (actual.month > alarma.month) tiempo_cumplido = true;
            else if (actual.month == alarma.month) {
                if (actual.mday > alarma.mday) tiempo_cumplido = true;
                else if (actual.mday == alarma.mday) {
                    if (actual.hour > alarma.hour) tiempo_cumplido = true;
                    else if (actual.hour == alarma.hour) {
                        if (actual.min > alarma.min) tiempo_cumplido = true;
                        else if (actual.min == alarma.min) {
                            if (actual.sec >= alarma.sec) tiempo_cumplido = true;
                        }
                    }
                }
            }
        }
        
        if (tiempo_cumplido) {
            break; // Salir del bucle y despertar
        }
        
        // Instrucción Assembler para detener el núcleo del procesador
        // hasta la próxima interrupción.
        __asm__ volatile ("wfi"); 
    }
}

// Algoritmo robusto para sumar tiempo sin librerías externas complejas
void rtc_sumar_segundos( rtc_t *origen, int segundos, rtc_t *destino ) {
    // Copiar origen a destino
    *destino = *origen;
    
    // Sumar segundos
    destino->sec += segundos;
    
    // Ajustar Segundos -> Minutos
    while( destino->sec >= 60 ) {
        destino->sec -= 60;
        destino->min++;
    }
    
    // Ajustar Minutos -> Horas
    while( destino->min >= 60 ) {
        destino->min -= 60;
        destino->hour++;
    }
    
    // Ajustar Horas -> Días
    while( destino->hour >= 24 ) {
        destino->hour -= 24;
        destino->mday++;
    }
    
    // Ajustar Días -> Meses (Simplificado para todos los meses 31 días por seguridad
    // o implementación básica 30/31. Para un datalogger esto es suficiente).
    // Nota: Si necesitas calendario gregoriano perfecto (bisiestos), se requiere más código.
    // Aquí asumimos mes genérico de 31 días para el rollover.
    while( destino->mday > 31 ) {
        destino->mday -= 31;
        destino->month++;
        // Ajuste básico de día 0 a 1 si el rollover deja en 0 (la struct rtc usa 1-31)
        if(destino->mday == 0) destino->mday = 1;
    }
    
    // Ajustar Meses -> Años
    while( destino->month > 12 ) {
        destino->month -= 12;
        destino->year++;
    }
}