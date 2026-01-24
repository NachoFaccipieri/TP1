#include "leer_adc.h"

uint8_t leerADC(uint8_t channel) {
    uint16_t valorADC;
    uint8_t  valorPorcentual;
    
    switch (channel){
        case 1: valorADC = adcRead(CH1);
                break;
        case 2: valorADC = adcRead(CH2);
                break;
        case 3: valorADC = adcRead(CH3);
                break;
    }
    
    valorPorcentual = 100 - (((float)valorADC / 1023.0f) * 100.0f);
    return valorPorcentual;
}
