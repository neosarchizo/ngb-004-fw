#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "g_log.h"
#include "n_fds.h"
#include "n_time.h"
#include "nrf_log.h"
#include <string.h>

APP_TIMER_DEF(m_g_log_timer);
APP_TIMER_DEF(m_g_log_fds_timer);

static volatile bool m_initiated = false;
static g_log_cb_t m_on_log_read = NULL;
static volatile uint8_t m_idx_log = 0;

static g_log_data m_data = {
    .length = 0,
};

static void g_log_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_on_log_read == NULL)
    {
        return;
    }

    m_on_log_read(m_data.logs[m_idx_log++]);

    if (m_idx_log < m_data.length)
    {
        app_timer_start(m_g_log_timer, APP_TIMER_TICKS(G_LOG_READ_INTERVAL_MS), NULL);
    }
    else
    {
        g_log_clear();
    }
}

static void g_log_fds_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = NRF_SUCCESS;

    err_code = n_fds_update(G_LOG_FDS_FILE_ID, G_LOG_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(g_log_data));
}

static ret_code_t fds_update()
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = app_timer_stop(m_g_log_fds_timer);
    err_code = app_timer_start(m_g_log_fds_timer, APP_TIMER_TICKS(G_LOG_FDS_UPDATE_DELAY_MS), NULL);

    return err_code;
}


ret_code_t g_log_init(g_log_cb_t on_log_read)
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = n_fds_find(G_LOG_FDS_FILE_ID, G_LOG_FDS_REC_KEY);

    if (err_code != NRF_SUCCESS)
    {
        err_code = n_fds_write(G_LOG_FDS_FILE_ID, G_LOG_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(g_log_data));
    }
    else
    {
        err_code = n_fds_open(G_LOG_FDS_FILE_ID, G_LOG_FDS_REC_KEY, (uint8_t *)&m_data, sizeof(g_log_data));
    }

    err_code = app_timer_create(&m_g_log_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                g_log_timeout);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_g_log_fds_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                g_log_fds_timeout);


    m_on_log_read = on_log_read;
    m_initiated = true;

    return err_code;
}

extern ret_code_t g_log_add()
{
    ret_code_t err_code = NRF_SUCCESS;

    if (!n_time_initiated())
    {
        return NRF_ERROR_INTERNAL;
    }

    n_time_data now;
    n_time_get(&now);

    g_log_time log_time = {
        .current_days = now.current_days,
        .hours = now.hours,
        .minutes = now.minutes,
        .seconds = now.seconds,
    };

    if (m_data.length < G_LOG_MAXIMUM_LENGTH)
    {
        m_data.logs[m_data.length++] = log_time;
    }
    else
    {
        memcpy((&m_data) + sizeof(uint8_t), (&m_data) + sizeof(uint8_t) + sizeof(g_log_time), (G_LOG_MAXIMUM_LENGTH - 1) * sizeof(g_log_time));
        m_data.logs[m_data.length - 1] = log_time;
    }

    err_code = fds_update();

    return err_code;
}

ret_code_t g_log_clear()
{
    ret_code_t err_code = NRF_SUCCESS;

    memset(&m_data, 0, sizeof(g_log_data));

    err_code = fds_update();

    return err_code;
}

ret_code_t g_log_get_data()
{
    m_idx_log = 0;

    if (m_data.length == 0)
    {
        return NRF_SUCCESS;
    }

    return app_timer_start(m_g_log_timer, APP_TIMER_TICKS(G_LOG_READ_INTERVAL_MS), NULL);
}

bool g_log_initiated()
{
    return m_initiated;
}