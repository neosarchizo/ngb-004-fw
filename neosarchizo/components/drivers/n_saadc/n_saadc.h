#ifndef N_SAADC_H_
#define N_SAADC_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*n_saadc_cb_t)(uint16_t value);

    extern ret_code_t n_saadc_measure(uint8_t pin, n_saadc_cb_t on_measured);
    
#ifdef __cplusplus
}
#endif

#endif /* N_FDS_H_ */