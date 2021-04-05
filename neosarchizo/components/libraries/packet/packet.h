#ifndef PACKET_H__
#define PACKET_H__

#include <stdint.h>
#include <stdbool.h>

#include <sdk_errors.h>
#include "nrf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PACKET_FRAME_HEADER 0xCF
#define PACKET_FRAME_FOOTER 0x25

#define PACKET_CMD_GET_BL_NAME 0x0
#define PACKET_CMD_GET_FW_VERSION 0x1
#define PACKET_CMD_SET_N_TIME 0x2
#define PACKET_CMD_GET_LOGS 0x3
#define PACKET_CMD_CLEAR_LOGS 0x4
#define PACKET_CMD_CLEAR_ALARMS 0x5
#define PACKET_CMD_ADD_ALARM 0x6
#define PACKET_CMD_BATTERY 0x7
#define PACKET_CMD_IR 0x8
#define PACKET_CMD_HALL 0x9
#define PACKET_CMD_BUTTON 0xa
#define PACKET_CMD_LED 0xb
#define PACKET_CMD_BUZZER 0xc
#define PACKET_CMD_CHARGING 0xd
#define PACKET_CMD_SET_ALARM_MUTED 0xe
#define PACKET_CMD_TRIG_ALARM 0xf
#define PACKET_CMD_OVERDOSE_ALARM 0x10

    extern ret_code_t packet_generate_packet(uint8_t cmd, uint8_t length, uint8_t *data);
    extern bool packet_check(uint8_t length, uint8_t *data);
    extern ret_code_t packet_get_data(uint8_t *src, uint8_t length, uint8_t *dst);
    extern uint8_t packet_get_length(uint8_t *data);
    extern uint8_t packet_get_cmd(uint8_t *data);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif // PACKET_H__
