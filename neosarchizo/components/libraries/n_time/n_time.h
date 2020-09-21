#ifndef N_TIME_H__
#define N_TIME_H__

#include <stdint.h>
#include <stdbool.h>

#include <sdk_errors.h>
#include "nrf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define N_TIME_WEEK_LENGTH 7
#define N_TIME_HOURS_LENGTH 24
#define N_TIME_MINUTES_LENGTH 60
#define N_TIME_SECONDS_LENGTH 60
#define N_TIME_INTERVAL APP_TIMER_TICKS(1000)

    typedef struct
    {
        uint16_t current_days; // from 2000-01-01
        uint8_t day_of_the_week;
        uint8_t hours;
        uint8_t minutes;
        uint8_t seconds;
    } n_time_data;

    typedef void (*n_time_cb_t)(n_time_data data);

    extern ret_code_t n_time_init(n_time_cb_t on_time_changed, n_time_cb_t on_minute_changed, n_time_cb_t on_day_changed);
    extern ret_code_t n_time_timer_start();
    extern bool n_time_initiated();
    extern ret_code_t n_time_set(n_time_data *data);
    extern ret_code_t n_time_get(n_time_data *data);

    /** @} */

#ifdef __cplusplus
}
#endif

#endif // N_TIME_H__
