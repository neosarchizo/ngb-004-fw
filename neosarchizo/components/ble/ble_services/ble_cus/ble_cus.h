#ifndef BLE_CUS_H__
#define BLE_CUS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_CUS_GATTS_SERVICE_UUID   				0x1912
#define BLE_CUS_GATTS_CHAR_CMD_UUID                             0x3C29

#define BLE_GATTS_CHAR_NUM		1

#define BLE_CUS_DEF(_name)                                                                          \
static ble_cus_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_CUS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus_on_ble_evt, &_name)

typedef struct ble_cus_s ble_cus_t;

typedef void (*ble_cb_t)(uint8_t *data, uint8_t length);

struct ble_cus_gatts_char_inst
{
   ble_gatts_char_handles_t handles;
   bool is_notify;
   ble_cb_t char_write_callback;
};

struct ble_cus_s
{
    uint16_t                      service_handle;                 /**< Handle of Battery Service (as provided by the BLE stack). */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
};

extern ret_code_t ble_cus_init(ble_cus_t *  p_cus);

extern ret_code_t ble_cus_set_callback_char_write(uint8_t char_id, ble_cb_t callback);

extern void ble_cus_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

extern ret_code_t ble_cus_char_write(ble_cus_t * p_cus, uint8_t char_id,  uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif // BLE_CUS_H__

/** @} */
