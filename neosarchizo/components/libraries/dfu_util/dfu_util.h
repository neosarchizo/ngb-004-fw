#ifndef DFU_UTIL_H__
#define DFU_UTIL_H__

#include <stdint.h>
#include <stdbool.h>

#include <sdk_errors.h>
#include "nrf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

// DFU
#define DFU_UTIL_DFU_ADV_PREFIX "SWB"
#define DFU_UTIL_FW_VERSION 1

    extern uint8_t dfu_util_get_dfu_adv_name(uint8_t *data);
    extern uint16_t dfu_util_get_fw_version();

    /** @} */

#ifdef __cplusplus
}
#endif

#endif // DFU_UTIL_H__
