#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "alarm.h"
#include "n_fds.h"
#include "n_time.h"
#include "buzzer.h"
#include "nrf_log.h"
#include "g_led.h"
#include "g_motor.h"
#include <string.h>

#define ALARM_NOTES_LENGTH 5

static buzzer_note m_alarm_notes[ALARM_NOTES_LENGTH] = {
    {
        .hertz = 2000,
        .duration = 50,
    },
    {
        .hertz = 0,
        .duration = 50,
    },
    {
        .hertz = 2000,
        .duration = 50,
    },
    {
        .hertz = 0,
        .duration = 50,
    },
    {
        .hertz = 0,
        .duration = 750,
    },
};

APP_TIMER_DEF(m_alarm_timer);
APP_TIMER_DEF(m_alarm_led_timer);
APP_TIMER_DEF(m_alarm_fds_timer);
APP_TIMER_DEF(m_snooze_timer);

static volatile bool m_alarming = false;
static alarm_cb_t m_on_alarm = NULL;

static alarm_data m_data = {
    .muted = ALARM_NOT_MUTED,
    .length = 0,
};

static volatile uint8_t m_idx_notes = 0;

static volatile uint8_t m_snooze_repeat = 0;
static volatile uint8_t m_snooze_interval = 0;

static void alarm_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = NRF_SUCCESS;

    m_alarming = false;
    g_led_set_mode(G_LED_MODE_OFF);
    g_motor_set_mode(G_MOTOR_MODE_OFF);

    NRF_LOG_INFO("alarm_timeout");

    NRF_LOG_INFO("m_snooze_repeat : %d", m_snooze_repeat);
    NRF_LOG_INFO("m_snooze_interval : %d", m_snooze_interval);

    if (m_snooze_repeat > 0)
    {
        --m_snooze_repeat;

        if (m_snooze_interval > 0)
        {
            err_code = app_timer_start(m_snooze_timer, APP_TIMER_TICKS(m_snooze_interval * ALARM_SNOOZE_INTERVAL_LENGTH_MS - ALARM_LENGTH_MS), NULL);
        }
    }
}

static void snooze_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = NRF_SUCCESS;

    NRF_LOG_INFO("m_snooze_repeat : %d", m_snooze_repeat)
    NRF_LOG_INFO("m_snooze_interval : %d", m_snooze_interval)

    err_code  = alarm_trig(m_data.muted, m_snooze_repeat, m_snooze_interval);
}

static void alarm_fds_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = NRF_SUCCESS;

    err_code = n_fds_update(ALARM_FDS_FILE_ID, ALARM_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(alarm_data));
}

static void alarm_led_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = NRF_SUCCESS;

    if (!m_alarming)
    {
        return;
    }

    for (uint8_t i = 0; i < ALARM_NOTES_LENGTH; i++)
    {
        if (i == m_idx_notes)
        {
            if (m_alarm_notes[i].hertz > 0)
            {
                g_led_set_mode(G_LED_MODE_WHITE);
                g_motor_set_mode(G_MOTOR_MODE_ON);
            }
            else
            {
                g_led_set_mode(G_LED_MODE_OFF);
                g_motor_set_mode(G_MOTOR_MODE_OFF);
            }

            if (m_alarming)
            {
                err_code = app_timer_start(m_alarm_led_timer, APP_TIMER_TICKS(m_alarm_notes[i].duration), NULL);
            }
            break;
        }
    }

    ++m_idx_notes;

    if (m_idx_notes == ALARM_NOTES_LENGTH)
    {
        m_idx_notes = 0;
    }
}

static ret_code_t fds_update()
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = app_timer_stop(m_alarm_fds_timer);
    err_code = app_timer_start(m_alarm_fds_timer, APP_TIMER_TICKS(ALARM_FDS_UPDATE_DELAY_MS), NULL);

    return err_code;
}

ret_code_t alarm_init(alarm_cb_t on_alarm)
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = n_fds_find(ALARM_FDS_FILE_ID, ALARM_FDS_REC_KEY);

    if (err_code != NRF_SUCCESS)
    {
        err_code = n_fds_write(ALARM_FDS_FILE_ID, ALARM_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(alarm_data));
    }
    else
    {
        err_code = n_fds_open(ALARM_FDS_FILE_ID, ALARM_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(alarm_data));
    }

    err_code = app_timer_create(&m_alarm_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                alarm_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_alarm_fds_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                alarm_fds_timeout);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_alarm_led_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                alarm_led_timeout);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_snooze_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                snooze_timeout);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    m_on_alarm = on_alarm;

    return err_code;
}

ret_code_t alarm_set_muted(bool muted)
{
    ret_code_t err_code = NRF_SUCCESS;

    m_data.muted = muted;

    err_code = fds_update();

    return err_code;
}

bool alarm_muted(void)
{
    return m_data.muted;
}

ret_code_t alarm_clear()
{
    ret_code_t err_code = NRF_SUCCESS;

    uint8_t muted = m_data.muted;
    memset(&m_data, 0, sizeof(alarm_data));
    m_data.muted = muted;
    err_code = fds_update();
    return err_code;
}

ret_code_t alarm_add(alarm_time *time)
{
    NRF_LOG_INFO("alarm_add : start");
    ret_code_t err_code = NRF_SUCCESS;

    if (time->partitions > ALARM_PARTITIONS_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_PARTITIONS_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (time->hours > ALARM_HOURS_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_HOURS_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (time->minutes > ALARM_MINUTES_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_MINUTES_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (time->refill_alarm > ALARM_REFILL_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_REFILL_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (time->snooze_repeat > ALARM_SNOOZE_REPEAT_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_SNOOZE_REPEAT_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (time->snooze_interval > ALARM_SNOOZE_INTERVAL_MAX_VALUE)
    {
        NRF_LOG_INFO("alarm_add : error (ALARM_SNOOZE_INTERVAL_MAX_VALUE)");
        return NRF_ERROR_INTERNAL;
    }

    if (m_data.length < ALARM_MAXIMUM_LENGTH)
    {
        memcpy(&m_data.alarms[m_data.length++], time, sizeof(alarm_time));
        NRF_LOG_INFO("alarm_add %d:", m_data.length);
    }
    else
    {
        err_code = NRF_ERROR_INTERNAL;
        return err_code;
    }

    err_code = fds_update();

    NRF_LOG_INFO("alarm_add : end");
    return err_code;
}

ret_code_t alarm_get_data(alarm_data *p_data)
{
    ret_code_t err_code = NRF_SUCCESS;
    memcpy(p_data, &m_data, sizeof(alarm_data));
    return err_code;
}

static bool is_alarm(alarm_time time, n_time_data now, qre1113gr_data ir_data)
{
    NRF_LOG_INFO("is_alarm : start");

    NRF_LOG_INFO("a : %d", ir_data.a);
    NRF_LOG_INFO("b : %d", ir_data.b);
    NRF_LOG_INFO("c : %d", ir_data.c);
    NRF_LOG_INFO("d : %d", ir_data.d);

    NRF_LOG_INFO("snooze_repeat : %d", time.snooze_repeat);
    NRF_LOG_INFO("snooze_interval : %d", time.snooze_interval);

    // check refill alarm
    if (time.refill_alarm > 0)
    {
        alarm_time refill_time;

        memcpy(&refill_time, &time, sizeof(alarm_time));

        if (time.refill_alarm > refill_time.hours)
        {
            refill_time.hours += ALARM_HOURS_MAX_VALUE;
            refill_time.hours -= time.refill_alarm;

            refill_time.day_of_the_week = (0b01111111 & (refill_time.day_of_the_week >> 1)) | (0b10000000 & (refill_time.day_of_the_week << 7));
        }
        else
        {
            refill_time.hours -= time.refill_alarm;
        }

        if (refill_time.day_of_the_week & (1 << now.day_of_the_week) && refill_time.hours == now.hours && refill_time.minutes == now.minutes)
        {
            // TODO partition
            bool empty_partition_existed = false;

            for (uint8_t i = 0; i < ALARM_PARTITIONS_MAX_POSITION; i++)
            {
                if ((refill_time.partitions & (1 << i)) > 0 && *(((uint8_t *)&ir_data) + i) == 0)
                {
                    empty_partition_existed = true;
                    break;
                }
            }

            return empty_partition_existed;
        }
    }

    // alarm

    // snooze
    if (!(time.day_of_the_week & (1 << now.day_of_the_week)))
    {
        return false;
    }

    if (time.hours != now.hours)
    {
        return false;
    }

    return time.minutes == now.minutes;
}

ret_code_t alarm_check(n_time_data time, qre1113gr_data ir_data)
{
    ret_code_t err_code = NRF_SUCCESS;

    NRF_LOG_INFO("alarm_check %d:", m_data.length);

    for (uint8_t i = 0; i < m_data.length; i++)
    {
        NRF_LOG_INFO("alarm %d: (%d) %02d-%02d", i, m_data.alarms[i].day_of_the_week, m_data.alarms[i].hours, m_data.alarms[i].minutes);

        if (is_alarm(m_data.alarms[i], time, ir_data))
        {
            err_code = alarm_trig(m_data.muted, m_data.alarms[i].snooze_repeat, m_data.alarms[i].snooze_interval);
        }
    }

    return err_code;
}

bool alarm_alarming(void)
{
    return m_alarming;
}

ret_code_t alarm_cancel(void)
{
    ret_code_t err_code = NRF_SUCCESS;

    m_alarming = false;
    g_led_set_mode(G_LED_MODE_OFF);
    g_motor_set_mode(G_MOTOR_MODE_OFF);
    m_snooze_repeat = 0;
    m_snooze_interval = 0;

    err_code = buzzer_stop();
    err_code = app_timer_stop(m_alarm_timer);
    err_code = app_timer_stop(m_snooze_timer);

    return err_code;
}

ret_code_t alarm_trig(bool muted, uint8_t snooze_repeat, uint8_t snooze_interval)
{
    ret_code_t err_code = NRF_SUCCESS;

    // TODO g_led
    m_alarming = true;
    m_snooze_repeat = snooze_repeat;
    m_snooze_interval = snooze_interval;

    NRF_LOG_INFO("m_snooze_repeat : %d", m_snooze_repeat);
    NRF_LOG_INFO("m_snooze_interval : %d", m_snooze_interval);

    err_code = app_timer_stop(m_alarm_timer);
    err_code = app_timer_stop(m_snooze_timer);

    if (!muted)
    {
        err_code = buzzer_play(m_alarm_notes, ALARM_NOTES_LENGTH, ALARM_LENGTH_MS);
    }

    if (m_on_alarm != NULL)
    {
        m_on_alarm();
    }

    err_code = app_timer_start(m_alarm_timer, APP_TIMER_TICKS(ALARM_LENGTH_MS), NULL);

    m_idx_notes = 0;

    err_code = app_timer_start(m_alarm_led_timer, APP_TIMER_TICKS(1), NULL);

    return err_code;
}