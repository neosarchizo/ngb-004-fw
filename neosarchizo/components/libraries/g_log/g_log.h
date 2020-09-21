#ifndef G_LOG_H__
#define G_LOG_H__

#include <stdint.h>
#include <stdbool.h>

#include <sdk_errors.h>
#include "nrf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define G_LOG_MAXIMUM_LENGTH 0xff

#define G_LOG_FDS_FILE_ID 0x0001
#define G_LOG_FDS_REC_KEY 0x0001
#define G_LOG_FDS_UPDATE_DELAY_MS 3000
#define G_LOG_READ_INTERVAL_MS 20

    typedef struct
    {
        uint16_t current_days; // from 2000-01-01
        uint8_t hours;
        uint8_t minutes;
        uint8_t seconds;
    } g_log_time;

    typedef struct
    {
        uint8_t length;
        g_log_time logs[G_LOG_MAXIMUM_LENGTH];
    } g_log_data;

    typedef void (*g_log_cb_t)(g_log_time data);

    extern ret_code_t g_log_init(g_log_cb_t on_log_read);
    extern ret_code_t g_log_add();
    extern ret_code_t g_log_clear();
    extern ret_code_t g_log_get_data();
    extern bool g_log_initiated();

    // TODO get

    /** @} */

#ifdef __cplusplus
}
#endif

#endif // G_LOG_H__
