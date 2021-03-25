#include "sdk_config.h"

#include "dfu_util.h"
#include "ble.h"
#include "nrf_log.h"

static void insert_hex_string(char * text, uint8_t value) {
    char hex_numbers[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    uint8_t quotient = value / 16;
    uint8_t remainder = value % 16;

    text[0] = hex_numbers[quotient];
    text[1] = hex_numbers[remainder];
}

uint8_t dfu_util_get_dfu_adv_name(uint8_t *data)
{
    ret_code_t err_code;

    char name[strlen(DFU_UTIL_DFU_ADV_PREFIX) + 4];

    for (uint8_t i = 0; i < strlen(DFU_UTIL_DFU_ADV_PREFIX); i++)
    {
        name[i] = DFU_UTIL_DFU_ADV_PREFIX[i];
    }

    ble_gap_addr_t addr;

    err_code = sd_ble_gap_addr_get(&addr);
    if (err_code != NRF_SUCCESS)
    {
        return 0;
    }
    
    insert_hex_string(name + strlen(DFU_UTIL_DFU_ADV_PREFIX), addr.addr[1]);
    addr.addr[0] += 1;
    insert_hex_string(name + strlen(DFU_UTIL_DFU_ADV_PREFIX) + 2, addr.addr[0]);

    memcpy(data, name, sizeof name);

    return sizeof name;
}

uint16_t dfu_util_get_fw_version() {
    return DFU_UTIL_FW_VERSION;
}