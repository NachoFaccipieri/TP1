#ifndef __SX126X_HAL_CONTEXT_H__
#define __SX126X_HAL_CONTEXT_H__

#include "sapi.h"

// Contexto para el HAL del SX126x
typedef struct {
    gpioMap_t nss_pin;
    gpioMap_t busy_pin;
    gpioMap_t reset_pin;
    gpioMap_t dio1_pin;
} sx126x_hal_context_t;

#endif // __SX126X_HAL_CONTEXT_H__
