#include "sdk_config.h"

#include "packet.h"
#include "nrf_log.h"

ret_code_t packet_generate_packet(uint8_t cmd, uint8_t length, uint8_t *data)
{
    // frame header
    *data = PACKET_FRAME_HEADER;

    // CMD
    *(data + 1) = cmd;

    // length
    *(data + 2) = length - 5;

    // checksum
    *(data + length - 2) = *data;

    for (uint8_t i = 1; i < length - 2; ++i)
    {
        *(data + length - 2) ^= *(data + i);
    }

    *(data + length - 1) = PACKET_FRAME_FOOTER;

    return NRF_SUCCESS;
}

bool packet_check(uint8_t length, uint8_t *data)
{
    if (*data != PACKET_FRAME_HEADER)
    {
        NRF_LOG_INFO("packet_check failed : frame header is not correct (%X) / (%X)", PACKET_FRAME_HEADER, *data);
        return false;
    }

    if (*(data + length - 1) != PACKET_FRAME_FOOTER)
    {
        NRF_LOG_INFO("packet_check failed : frame footer is not correct (%X) / (%X)", PACKET_FRAME_FOOTER, *(data + length));
        return false;
    }

    switch (*(data + 1))
    {
    case PACKET_CMD_IR:
    case PACKET_CMD_HALL:
    case PACKET_CMD_BUTTON:
    case PACKET_CMD_LED:
    case PACKET_CMD_BUZZER:
    case PACKET_CMD_SET_N_TIME:
    case PACKET_CMD_BATTERY:
    case PACKET_CMD_CHARGING:
    case PACKET_CMD_GET_LOGS:
    case PACKET_CMD_CLEAR_LOGS:
    case PACKET_CMD_CLEAR_ALARMS:
    case PACKET_CMD_ADD_ALARM:
    case PACKET_CMD_SET_ALARM_MUTED:
    case PACKET_CMD_TRIG_ALARM:
    case PACKET_CMD_OVERDOSE_ALARM:
        break;
    default:
        NRF_LOG_INFO("packet_check failed : cmd is not correct (%X)", *(data + 1));
        return false;
    }

    uint8_t cs = *data;

    for (uint8_t i = 1; i < length - 2; ++i)
    {
        cs ^= *(data + i);
    }

    if (cs != *(data + length - 2))
    {
        NRF_LOG_INFO("packet_check failed : checksum is not correct (%X) / (%X)", cs, *(data + length - 2));
        return false;
    }

    return true;
}

ret_code_t packet_get_data(uint8_t *src, uint8_t length, uint8_t *dst)
{

    for (uint8_t i = 0; i < length; ++i)
    {
        *(dst + i) = *(src + 3 + i);
    }

    return NRF_SUCCESS;
}

uint8_t packet_get_length(uint8_t *data)
{
    return *(data + 2);
}

uint8_t packet_get_cmd(uint8_t *data)
{
    return *(data + 1);
}

/*lint --flb "Leave library region" */
