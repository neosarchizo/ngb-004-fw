#ifndef MULTIPLEXER_H_
#define MULTIPLEXER_H_

#include <stdint.h>
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MULTIPLEXER_PIN_EN 6
#define MULTIPLEXER_PIN_S0 3
#define MULTIPLEXER_PIN_S1 4
#define MULTIPLEXER_PIN_S2 5

#define MULTIPLEXER_VALUE_MAX 8

    extern ret_code_t multiplexer_init();
    extern void multiplexer_set_enabled(bool enabled);
    extern void multiplexer_set_bit(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLEXER_H_ */