#ifndef ALARM_H_
#define ALARM_H_

#include <stdint.h>
#include <stdbool.h>

#include <sdk_errors.h>
#include "nrf_error.h"
#include "n_time.h"
#include "qre1113gr.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ALARM_MAXIMUM_LENGTH 10

#define ALARM_FDS_FILE_ID 0x0000
#define ALARM_FDS_REC_KEY 0x0001
#define ALARM_FDS_UPDATE_DELAY_MS 3000

#define ALARM_MUTED 1
#define ALARM_NOT_MUTED 0

#define ALARM_PARTITIONS_MAX_VALUE 0b1111
#define ALARM_PARTITIONS_MAX_POSITION 4

#define ALARM_HOURS_MAX_VALUE 23
#define ALARM_MINUTES_MAX_VALUE 59

#define ALARM_REFILL_MAX_VALUE 23

#define ALARM_SNOOZE_REPEAT_MAX_VALUE 5
#define ALARM_SNOOZE_INTERVAL_MAX_VALUE 30

#define ALARM_LENGTH_MS 30000
#define ALARM_SNOOZE_INTERVAL_LENGTH_MS 60000

    typedef struct
    {
        uint8_t partitions;
        uint8_t day_of_the_week;
        uint8_t hours;
        uint8_t minutes;
        uint8_t refill_alarm; // -1 ~ -23
        uint8_t snooze_repeat; // 1, 3, 5
        uint8_t snooze_interval; // 5, 10, 15, 30
    } alarm_time;

    typedef struct
    {
        uint8_t muted;
        uint8_t length;
        alarm_time alarms[ALARM_MAXIMUM_LENGTH];
    } alarm_data;
    
    typedef void (*alarm_cb_t)(void);

    extern ret_code_t alarm_init(alarm_cb_t on_alarm);
    extern ret_code_t alarm_set_muted(bool muted);
    extern bool alarm_muted(void);
    extern ret_code_t alarm_clear();
    extern ret_code_t alarm_add(alarm_time * time);
    extern ret_code_t alarm_get_data(alarm_data * p_data);
    extern ret_code_t alarm_check(n_time_data time, qre1113gr_data ir_data);
    extern bool alarm_alarming(void);
    extern ret_code_t alarm_cancel(void);
    extern ret_code_t alarm_trig(bool muted, uint8_t snooze_repeat, uint8_t snooze_interval);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_H_ */