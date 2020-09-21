#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "n_time.h"
#include "nrf_log.h"
#include <string.h>

APP_TIMER_DEF(m_n_time_timer);

static n_time_data m_current_time;
static volatile bool m_initiated = false;
static n_time_cb_t m_on_time_changed = NULL;
static n_time_cb_t m_on_minute_changed = NULL;
static n_time_cb_t m_on_day_changed = NULL;

static ret_code_t n_time_tick()
{
    if (!m_initiated)
        return NRF_ERROR_INTERNAL;

    bool minute_changed = false;
    bool day_changed = false;

    if (++m_current_time.seconds == N_TIME_SECONDS_LENGTH)
    {
        m_current_time.seconds = 0;
        minute_changed = true;

        if (++m_current_time.minutes == N_TIME_MINUTES_LENGTH)
        {
            m_current_time.minutes = 0;

            if (++m_current_time.hours == N_TIME_HOURS_LENGTH)
            {
                m_current_time.hours = 0;
                ++m_current_time.current_days;

                if (++m_current_time.day_of_the_week == N_TIME_WEEK_LENGTH)
                {
                    m_current_time.day_of_the_week = 0;
                }

                day_changed = true;
            }
        }
    }

    if (minute_changed && m_on_minute_changed != NULL)
    {
        m_on_minute_changed(m_current_time);
    }

    if (day_changed && m_on_day_changed != NULL)
    {
        m_on_day_changed(m_current_time);
    }

    return NRF_SUCCESS;
}

static void n_time_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    n_time_tick();

    if (!n_time_initiated())
    {
        return;
    }

    if (m_on_time_changed == NULL)
    {
        return;
    }

    n_time_data data;
    n_time_get(&data);

    m_on_time_changed(data);
}

ret_code_t n_time_init(n_time_cb_t on_time_changed, n_time_cb_t on_minute_changed, n_time_cb_t on_day_changed)
{
    ret_code_t err_code = app_timer_create(&m_n_time_timer,
                                           APP_TIMER_MODE_REPEATED,
                                           n_time_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    m_on_time_changed = on_time_changed;
    m_on_minute_changed = on_minute_changed;
    m_on_day_changed = on_day_changed;

    return err_code;
}

bool n_time_initiated()
{
    return m_initiated;
}

ret_code_t n_time_timer_start()
{
    return app_timer_start(m_n_time_timer, N_TIME_INTERVAL, NULL);
}

ret_code_t n_time_set(n_time_data *data)
{
    ret_code_t err_code = NRF_SUCCESS;

    // check values
    if (data->day_of_the_week >= N_TIME_WEEK_LENGTH)
        return NRF_ERROR_INTERNAL;

    if (data->hours >= N_TIME_HOURS_LENGTH)
        return NRF_ERROR_INTERNAL;

    if (data->minutes >= N_TIME_MINUTES_LENGTH)
        return NRF_ERROR_INTERNAL;

    if (data->seconds >= N_TIME_SECONDS_LENGTH)
        return NRF_ERROR_INTERNAL;

    if (!m_initiated)
    {
        m_initiated = true;
    }

    memcpy(&m_current_time, data, sizeof(m_current_time));

    return NRF_SUCCESS;
}

ret_code_t n_time_get(n_time_data *data)
{
    memcpy(data, &m_current_time, sizeof(m_current_time));
}