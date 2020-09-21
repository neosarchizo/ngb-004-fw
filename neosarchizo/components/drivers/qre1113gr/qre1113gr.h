#ifndef QRE1113GR_H_
#define QRE1113GR_H_

#include <stdint.h>
#include "app_util_platform.h"
#include "nrf_drv_saadc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QRE1113GR_PIN_A_COL NRF_SAADC_INPUT_AIN0
#define QRE1113GR_PIN_B_COL NRF_SAADC_INPUT_AIN1
#define QRE1113GR_PIN_C_COL NRF_SAADC_INPUT_AIN2
#define QRE1113GR_PIN_D_COL NRF_SAADC_INPUT_AIN3

#define QRE1113GR_PIN_A_K 9
#define QRE1113GR_PIN_B_K 16
#define QRE1113GR_PIN_C_K 14
#define QRE1113GR_PIN_D_K 11

// #define QRE1113GR_DETECT_DISTANCE 14100
#define QRE1113GR_DETECT_DISTANCE 13600

#define QRE1113GR_READ_INTERVAL_MS 2
#define QRE1113GR_WAIT_INTERVAL_MS 100

    typedef struct
    {
        uint8_t a;
        uint8_t b;
        uint8_t c;
        uint8_t d;
    } qre1113gr_data;

    typedef void (*qre1113gr_cb_t)(qre1113gr_data data);

    extern ret_code_t qre1113gr_init(qre1113gr_cb_t on_ir_measured);
    extern void qre1113gr_read(qre1113gr_data * data); 
    extern ret_code_t qre1113gr_update(); 

#ifdef __cplusplus
}
#endif

#endif /* QRE1113GR_H_ */