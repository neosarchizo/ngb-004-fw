#include "sdk_common.h"

#define BLE_CUS_ENABLED 1

#if NRF_MODULE_ENABLED(BLE_CUS)
#include "ble_cus.h"
#include "ble_srv_common.h"
#include <string.h>

static ret_code_t char_cmd_add(ble_cus_t *p_cus);

static struct ble_cus_gatts_char_inst gl_char[BLE_GATTS_CHAR_NUM] = {
    {
        // cmd
        .is_notify = false,
        .char_write_callback = NULL,
    }};

ret_code_t ble_cus_init(ble_cus_t *p_cus) {
  if (p_cus == NULL) {
    return NRF_ERROR_NULL;
  }

  ret_code_t err_code;
  ble_uuid_t ble_uuid;

  // Initialize service structure
  p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;

  // add service
  BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_CUS_GATTS_SERVICE_UUID);

  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // add chars
  err_code = char_cmd_add(p_cus);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  return err_code;
}

void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
  if ((p_context == NULL) || (p_ble_evt == NULL)) {
    return;
  }

  ble_cus_t *p_cus = (ble_cus_t *)p_context;

  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED: {
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    break;
  }
  case BLE_GAP_EVT_DISCONNECTED: {
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
    break;
  }
  case BLE_GATTS_EVT_WRITE: {
    ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    for (uint8_t i = 0; i < BLE_GATTS_CHAR_NUM; i++) {
      if ((p_evt_write->handle == gl_char[i].handles.cccd_handle) && (p_evt_write->len == 2)) {
        gl_char[i].is_notify = ble_srv_is_notification_enabled(p_evt_write->data);
      }
    }

    for (uint8_t i = 0; i < BLE_GATTS_CHAR_NUM; i++) {
      if (p_evt_write->handle == gl_char[i].handles.value_handle) {
        if (gl_char[i].char_write_callback != NULL) {
          ((void (*)(uint8_t *, uint8_t))gl_char[i].char_write_callback)((uint8_t *)p_evt_write->data, p_evt_write->len);
        }
      }
    }
    break;
  }
  default:
    break;
  }
}

ret_code_t ble_cus_set_callback_char_write(uint8_t char_id, ble_cb_t callback) {
  ret_code_t err_code = NRF_SUCCESS;
  if (char_id >= BLE_GATTS_CHAR_NUM) {
    err_code = NRF_ERROR_BASE_NUM;
    return err_code;
  }
  gl_char[char_id].char_write_callback = callback;
  return err_code;
}

static ret_code_t char_cmd_add(ble_cus_t *p_cus) {
  ret_code_t err_code;
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t attr_char_value;
  ble_uuid_t ble_uuid;
  ble_gatts_attr_md_t attr_md;

  // notification
  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
  cccd_md.vloc = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.notify = 1;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;
  char_md.p_cccd_md = &cccd_md;
  char_md.p_sccd_md = NULL;

  BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_CUS_GATTS_CHAR_CMD_UUID);

  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.write_perm);

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 1;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = 20;
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = 20;

  err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle,
      &char_md,
      &attr_char_value,
      &gl_char[0].handles);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  return NRF_SUCCESS;
}

ret_code_t ble_cus_char_write(ble_cus_t *p_cus, uint8_t char_id, uint8_t *data, uint8_t length) {
  if (p_cus == NULL) {
    return NRF_ERROR_NULL;
  }

  ret_code_t err_code = NRF_SUCCESS;

  if (char_id >= BLE_GATTS_CHAR_NUM) {
    err_code = NRF_ERROR_BASE_NUM;
    return err_code;
  }

  if (data == NULL) {
    err_code = NRF_ERROR_BASE_NUM;
    return err_code;
  }

  if (length == 0) {
    err_code = NRF_ERROR_BASE_NUM;
    return err_code;
  }

  ble_gatts_value_t gatts_value;

  // Initialize value struct.
  memset(&gatts_value, 0, sizeof(gatts_value));

  gatts_value.len = length;
  gatts_value.offset = 0;
  gatts_value.p_value = data;

  // Update database.
  err_code = sd_ble_gatts_value_set(p_cus->conn_handle,
      gl_char[char_id].handles.value_handle,
      &gatts_value);

  if (err_code != NRF_SUCCESS) {
    NRF_LOG_INFO("ble_cus_char_write failed : sd_ble_gatts_value_set");
    NRF_LOG_INFO("err_code : %X", err_code);
    return err_code;
  }

  if (p_cus->conn_handle != BLE_CONN_HANDLE_INVALID && gl_char[char_id].is_notify) {
    ble_gatts_hvx_params_t hvx_params;

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = gl_char[char_id].handles.value_handle;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = gatts_value.offset;
    hvx_params.p_len = &gatts_value.len;
    hvx_params.p_data = gatts_value.p_value;

    err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);

    if (err_code != NRF_SUCCESS) {
      NRF_LOG_INFO("ble_cus_char_write failed : sd_ble_gatts_hvx");
      NRF_LOG_INFO("err_code : %X", err_code);
      return err_code;
    }
    return err_code;
  } else {
    err_code = NRF_ERROR_INVALID_STATE;
    NRF_LOG_INFO("ble_cus_char_write failed");
    NRF_LOG_INFO("err_code : %X", err_code);
    return err_code;
  }

  return err_code;
}

#endif // NRF_MODULE_ENABLED(BLE_CUS)
