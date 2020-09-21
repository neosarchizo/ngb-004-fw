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

#define PACKET_CMD_IR 0x00
#define PACKET_CMD_HALL 0x01
#define PACKET_CMD_BUTTON 0x02
#define PACKET_CMD_LED 0x03
#define PACKET_CMD_BUZZER 0x04
#define PACKET_CMD_SET_N_TIME 0x05
#define PACKET_CMD_BATTERY 0x06
#define PACKET_CMD_CHARGING 0x07
#define PACKET_CMD_GET_LOGS 0x08
#define PACKET_CMD_CLEAR_LOGS 0x09
#define PACKET_CMD_CLEAR_ALARMS 0x10
#define PACKET_CMD_ADD_ALARM 0x11
#define PACKET_CMD_SET_ALARM_MUTED 0x12
#define PACKET_CMD_TRIG_ALARM 0x13
#define PACKET_CMD_OVERDOSE_ALARM 0x14

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
