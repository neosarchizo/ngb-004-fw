#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "qre1113gr.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "n_saadc.h"
#include "multiplexer.h"

APP_TIMER_DEF(m_qre1113gr_timer);

static qre1113gr_data m_data;
static qre1113gr_data m_temp_data;
static volatile bool m_reading = false;
static volatile uint8_t m_idx_reading = 0;
static qre1113gr_cb_t m_on_ir_measured = NULL;

static void on_a_measured(uint16_t value)
{
    NRF_LOG_INFO("on_a_measured : %d", value);

    m_temp_data.a = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_A_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_b_measured(uint16_t value)
{
    NRF_LOG_INFO("on_b_measured : %d", value);

    m_temp_data.b = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_B_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_c_measured(uint16_t value)
{
    NRF_LOG_INFO("on_c_measured : %d", value);

    m_temp_data.c = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_C_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_d_measured(uint16_t value)
{
    NRF_LOG_INFO("on_d_measured : %d", value);

    m_temp_data.d = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_D_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_e_measured(uint16_t value)
{
    NRF_LOG_INFO("on_e_measured : %d", value);

    m_temp_data.e = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_E_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_f_measured(uint16_t value)
{
    NRF_LOG_INFO("on_f_measured : %d", value);

    m_temp_data.f = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_F_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_g_measured(uint16_t value)
{
    NRF_LOG_INFO("on_g_measured : %d", value);

    m_temp_data.g = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_G_K, 1);

    app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_WAIT_INTERVAL_MS), NULL);
}

static void on_h_measured(uint16_t value)
{
    NRF_LOG_INFO("on_h_measured : %d", value);

    m_temp_data.h = value < QRE1113GR_DETECT_DISTANCE;

    nrf_gpio_pin_write(QRE1113GR_PIN_H_K, 1);
    multiplexer_set_enabled(false);

    memcpy(&m_data, &m_temp_data, sizeof(qre1113gr_data));

    if (m_on_ir_measured != NULL)
    {
        m_on_ir_measured(m_data);
    }

    m_reading = false;
}

static void qre1113gr_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code;

    switch (m_idx_reading++)
    {
    case 0:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_a_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 1:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_B_K, 0);
        multiplexer_set_bit(1);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }
    case 2:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_b_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 3:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_C_K, 0);
        multiplexer_set_bit(2);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }
    case 4:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_c_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 5:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_D_K, 0);
        multiplexer_set_bit(3);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }

    case 6:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_d_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 7:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_E_K, 0);
        multiplexer_set_bit(3);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }

    case 8:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_e_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 9:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_F_K, 0);
        multiplexer_set_bit(3);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }

    case 10:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_f_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 11:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_G_K, 0);
        multiplexer_set_bit(3);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }

    case 12:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_g_measured);
        APP_ERROR_CHECK(err_code);
        break;
    }
    case 13:
    {
        nrf_gpio_pin_write(QRE1113GR_PIN_H_K, 0);
        multiplexer_set_bit(3);

        app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
        break;
    }

    case 14:
    {
        err_code = n_saadc_measure(QRE1113GR_PIN_COL, on_h_measured);
        APP_ERROR_CHECK(err_code);

        break;
    }
    default:
        break;
    }
}

ret_code_t qre1113gr_init(qre1113gr_cb_t on_ir_measured)
{
    ret_code_t err_code = NRF_SUCCESS;

    nrf_gpio_cfg_output(QRE1113GR_PIN_A_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_B_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_C_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_D_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_E_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_F_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_G_K);
    nrf_gpio_cfg_output(QRE1113GR_PIN_H_K);

    nrf_gpio_pin_write(QRE1113GR_PIN_A_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_B_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_C_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_D_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_E_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_F_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_G_K, 1);
    nrf_gpio_pin_write(QRE1113GR_PIN_H_K, 1);

    err_code = app_timer_create(&m_qre1113gr_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                qre1113gr_timeout);

    m_on_ir_measured = on_ir_measured;

    return err_code;
}

void qre1113gr_read(qre1113gr_data *data)
{
    memcpy(data, (uint8_t *)&m_data, sizeof(qre1113gr_data));
}

ret_code_t qre1113gr_update()
{
    if (m_reading)
    {
        return NRF_ERROR_INTERNAL;
    }

    m_idx_reading = 0;

    m_reading = true;
    multiplexer_set_enabled(true);
    multiplexer_set_bit(0);

    nrf_gpio_pin_write(QRE1113GR_PIN_A_K, 0);

    return app_timer_start(m_qre1113gr_timer, APP_TIMER_TICKS(QRE1113GR_READ_INTERVAL_MS), NULL);
}