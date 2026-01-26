#include "leer_adc.h"

uint8_t leerADC(uint8_t channel) {
    uint16_t valorADC;
    uint8_t  valorPorcentual;
    
    // Leer directamente el canal (CH1, CH2, CH3 son defines de sapi)
    valorADC = adcRead(channel);
    
    valorPorcentual = 100 - (((float)valorADC / 1023.0f) * 100.0f);
    return valorPorcentual;
}
