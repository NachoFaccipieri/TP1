/*
 * EDU-CIAA -> Heltec (por UART software GPIO1 con timer preciso)
 * 
 * Lee sensores y envía datos al Heltec por UART software en GPIO1
 * CON MODO BAJO CONSUMO usando RTC y __WFI()
 */

#include "sapi.h"
#include "leer_adc.h"
#include "sapi_dht11.h"

#define TX_PIN  GPIO1  // Pin 32 - TX hacia Heltec
#define DHT11_PIN GPIO7 // Pin para el sensor DHT11
#define TIEMPO_SLEEP 15 // Tiempo a dormir en SEGUNDOS (15 para demo en facultad, 300-600 para producción)

// Delay preciso usando delay() para 300 baud
void preciseBitDelay(void) {
    // 300 baud = 3.33 ms por bit (ultra lento pero estable)
    delay(4);  // 3 milisegundos, muy estable
}

// Enviar un byte por UART software
void softUART_writeByte(uint8_t data) {
    // START bit (LOW)
    gpioWrite(TX_PIN, LOW);
    preciseBitDelay();
    
    // 8 bits de datos (LSB primero)
    for(int i = 0; i < 8; i++) {
        if(data & (1 << i)) {
            gpioWrite(TX_PIN, HIGH);
        } else {
            gpioWrite(TX_PIN, LOW);
        }
        preciseBitDelay();
    }
    
    // STOP bit (HIGH)
    gpioWrite(TX_PIN, HIGH);
    preciseBitDelay();
}

void softUART_writeString(const char* str) {
    while(*str) {
        softUART_writeByte(*str);
        str++;
    }
}

// ===== FUNCIONES DE BAJO CONSUMO =====

// Sleep con WFI para bajo consumo
void dormir_bajo_consumo(uint32_t segundos) {
    printf("Durmiendo por %d segundos con WFI...", segundos);
    
    // Dividir en intervalos de 500ms para no saturar el loop
    uint32_t intervalos = segundos * 2; // 2 intervalos por segundo
    
    for(uint32_t i = 0; i < intervalos; i++) {
        delay(500); // 500ms
        __asm__ volatile ("wfi"); // CPU duerme entre delays
    }
    
    printf(" Despierto!\r\n");
}

// Estas funciones ya no se usan pero las dejo comentadas por si acaso
/*
// Sumar segundos al RTC de forma robusta
void rtc_sumar_segundos(rtc_t *origen, int segundos, rtc_t *destino) {
    *destino = *origen;
    
    destino->sec += segundos;
    
    // Ajustar Segundos -> Minutos
    while(destino->sec >= 60) {
        destino->sec -= 60;
        destino->min++;
    }
    
    // Ajustar Minutos -> Horas
    while(destino->min >= 60) {
        destino->min -= 60;
        destino->hour++;
    }
    
    // Ajustar Horas -> Días
    while(destino->hour >= 24) {
        destino->hour -= 24;
        destino->mday++;
    }
    
    // Ajustar Días -> Meses (simplificado: 31 días por mes)
    while(destino->mday > 31) {
        destino->mday -= 31;
        destino->month++;
        if(destino->mday == 0) destino->mday = 1;
    }
    
    // Ajustar Meses -> Años
    while(destino->month > 12) {
        destino->month -= 12;
        destino->year++;
    }
}

// Dormir el sistema usando __WFI() hasta que el RTC alcance la alarma
void dormir_sistema_hasta_alarma(uint32_t segundos) {
    rtc_t actual, alarma;
    
    // Leer hora actual
    if (!rtcRead(&actual)) {
        printf("Error: RTC no responde, usando delay normal\r\n");
        delay(segundos * 1000);
        return;
    }
    
    printf("RTC actual: %02d:%02d:%02d\r\n", actual.hour, actual.min, actual.sec);
    
    // Calcular hora de despertar
    rtc_sumar_segundos(&actual, segundos, &alarma);
    printf("Despertar en: %02d:%02d:%02d\r\n", alarma.hour, alarma.min, alarma.sec);
    
    // Contador de seguridad
    uint32_t iteraciones = 0;
    uint32_t max_iter = segundos * 1100; // Margen de seguridad
    
    // Bucle de Sleep con debug cada segundo
    uint8_t ultimo_segundo = actual.sec;
    
    while(true) {
        if (!rtcRead(&actual)) {
            printf("RTC falló durante sleep\r\n");
            break;
        }
        
        // Debug cada segundo que pasa
        if (actual.sec != ultimo_segundo) {
            printf(".");
            ultimo_segundo = actual.sec;
        }
        
        iteraciones++;
        if (iteraciones > max_iter) {
            printf("\nTimeout alcanzado\r\n");
            break;
        }
        
        // Comprobar si llegamos al tiempo de alarma
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
            printf("\nAlarma alcanzada!\r\n");
            break;
        }
        
        // Wait For Interrupt - Apaga CPU hasta próxima interrupción
        __asm__ volatile ("wfi"); 
    }
}
*/

int main(void) {
    boardInit();
    
    // UART_USB para debug
    uartConfig(UART_USB, 115200);
    
    adcConfig(ADC_ENABLE);
    
    // Inicializar sensor DHT11
    dht11Init(DHT11_PIN);
      
    // Configurar GPIO1 como salida (TX)
    gpioInit(TX_PIN, GPIO_OUTPUT);
    gpioWrite(TX_PIN, HIGH); // Idle state
    
    printf("\n=== EDU-CIAA -> HELTEC (GPIO1 software UART) ===\r\n");
    printf("Sensores: Luz(CH3), HumSuelo(CH2), DHT11(GPIO7)\r\n");
    printf("MODO BAJO CONSUMO: Sleep %d segundos con WFI\r\n\n", TIEMPO_SLEEP);
    
    int contador = 0;
    
    while(1) {
        // Leer sensores analógicos (sin delays, el sleep ya da tiempo suficiente)
        uint8_t luz_percent = leerADC(CH3);
        uint8_t hum_suelo = leerADC(CH2);
        
        // Variables para DHT11
        float temperatura = 0;
        float humedad_aire = 0;
        
        // Leer DHT11 (temperatura y humedad del aire)
        bool dht_ok = dht11Read(&humedad_aire, &temperatura);
        
        // Convertir a enteros
        int temp_int = (int)temperatura;
        int hum_int = (int)humedad_aire;
        
        if (!dht_ok) {
            printf("Error leyendo DHT11 en GPIO7\r\n");
        }
        
        // Crear mensaje con todos los sensores
        char mensaje[100];
        sprintf(mensaje, "temp:%d,hum_air:%d,luz:%d,hum_soil:%d,count:%d\n", 
                temp_int, hum_int, luz_percent, hum_suelo, contador);
        
        // Debug en USB
        printf("[TX %d] %s", contador, mensaje);
        
        // Enviar por GPIO1 (software UART) al Heltec
        softUART_writeString(mensaje);
        
        contador++;
        
        // MODO BAJO CONSUMO: Sleep híbrido (delay + WFI)
        dormir_bajo_consumo(TIEMPO_SLEEP);
    }
    
    return 0;
}