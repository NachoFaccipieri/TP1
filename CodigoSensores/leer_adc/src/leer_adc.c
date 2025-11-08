/*=====[Inclusions of function dependencies]=================================*/

/*=====[Definition macros of private constants]==============================*/

/*=====[Definitions of extern global variables]==============================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Main function, program entry point after power on or reset]==========*/

/*
@brief Lee un valor del  adc
@param canal  que se va a leer
@return retorna en valor porcentual lo leido 
*/
   uint8_t leerADC(uint8_t channel) {
      uint16_t valorADC;
      uint8_t  valorPorcentual; // Variable para el porcentaje de 0 a 100
      // 1. Leemos el canal analógico seleccionado (ADC_CHN). Rango: 0 a 1023.
      switch channel{
            case 1: valorADC = adcRead(CH1);
                    break;
            case 2: valorADC = adcRead(CH2);
                    break;
            case 3: valorADC = adcRead(CH3);
                    break;
      }
      // 2. Mapeamos el valor del ADC a un porcentaje
      // Como un valor ADC ALTO significa valor porcentual bajo, debemos INVERTIR el resultado.
      // Un valor ADC cercano a 1023 será 0%.
      // Un valor ADC cercano a 0 será 100%.
      porcentajeHumedad = 100 - (((float)valorADC / 1023.0f) * 100.0f);
      return valorPorcentual;
   }
