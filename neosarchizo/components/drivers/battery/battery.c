#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "battery.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_drv_gpiote.h"
#include "n_saadc.h"

APP_TIMER_DEF(m_bat_timer);
APP_TIMER_DEF(m_bat_uncharged_timer);
APP_TIMER_DEF(m_bat_oversample_timer);
APP_TIMER_DEF(m_bat_charging_state_timer);
APP_TIMER_DEF(m_bat_charging_state_measure_timer);

static volatile uint8_t m_bat_level = 0;
static volatile uint8_t m_charging_state = BAT_CHARGING_OFF;
static volatile bool m_measuring = false;

static volatile uint32_t m_sum = 0;
static volatile uint8_t m_sample_count = 0;

static bat_cb_t m_on_measured = NULL;
static bat_cb_t m_on_charge_changed = NULL;

static void oversample_timeout(void *p_context);

static void on_saadc_measured(uint16_t value)
{
    uint16_t mv = ADC_RESULT_IN_MILLI_VOLTS(value) * VOLTAGE_DIVIDER_SCALING_COMPENSATION;

    NRF_LOG_INFO("mV : %d", mv);

    if (m_sample_count++ == 0)
    {
        m_sum = mv;
    }
    else
    {
        m_sum += mv;
    }

    if (m_sample_count == BAT_OVERSAMPLE)
    {
        uint32_t avg = m_sum / BAT_OVERSAMPLE;

        if (avg >= BAT_VOLTAGE_MV_MAXIMUM)
        {
            m_bat_level = 100;
        }
        else if (avg <= BAT_VOLTAGE_MV_MINIMUM)
        {
            m_bat_level = 0;
        }
        else
        {
            m_bat_level = 100 * (((float)avg - BAT_VOLTAGE_MV_MINIMUM) / (BAT_VOLTAGE_MV_MAXIMUM - BAT_VOLTAGE_MV_MINIMUM));
        }

        NRF_LOG_INFO("battery : %d", m_bat_level);

        if (m_on_measured != NULL)
        {
            m_on_measured(m_bat_level);
        }

        m_measuring = false;
        return;
    }

    app_timer_start(m_bat_oversample_timer, BAT_OVERSAMPLE_INTERVAL, NULL);
}

static void oversample_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = n_saadc_measure(BAT_ADC_PIN, on_saadc_measured);

    if (err_code != NRF_SUCCESS)
    {
        app_timer_start(m_bat_oversample_timer, BAT_OVERSAMPLE_INTERVAL, NULL);
    }
}

static void charging_state_measure_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    uint8_t state = nrf_gpio_pin_read(BAT_STAT_PIN);
    nrf_gpio_cfg_input(BAT_STAT_PIN, NRF_GPIO_PIN_NOPULL);

    if (m_charging_state == 0 && state == 1)
    {
        // charging off
        app_timer_start(m_bat_uncharged_timer, BAT_UNCHARGED_INTERVAL, NULL);

        if (m_on_charge_changed != NULL)
        {
            m_on_charge_changed(m_charging_state);
        }
    }
    else if (m_charging_state == 1 && state == 0)
    {
        // charging on
        if (m_on_charge_changed != NULL)
        {
            m_on_charge_changed(m_charging_state);
        }
    }

    m_charging_state = state;
}

static void charging_state_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    nrf_gpio_cfg_input(BAT_STAT_PIN, NRF_GPIO_PIN_PULLUP);
    app_timer_start(m_bat_charging_state_measure_timer, BAT_CHARGING_STATE_MEASURE_INTERVAL, NULL);
}

static void bat_measure()
{
    if (m_measuring)
    {
        return;
    }

    m_sample_count = 0;
    m_measuring = true;

    app_timer_start(m_bat_oversample_timer, BAT_OVERSAMPLE_INTERVAL, NULL);
}

static void bat_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_charging_state)
    {
        bat_measure();
    }
}

ret_code_t battery_init(bat_cb_t on_measured, bat_cb_t on_charge_changed)
{
    ret_code_t err_code = app_timer_create(&m_bat_timer,
                                           APP_TIMER_MODE_REPEATED,
                                           bat_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_bat_uncharged_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                bat_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_bat_oversample_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                oversample_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_bat_charging_state_timer,
                                APP_TIMER_MODE_REPEATED,
                                charging_state_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_create(&m_bat_charging_state_measure_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                charging_state_measure_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    nrf_gpio_cfg_input(BAT_STAT_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_delay_ms(BAT_CHARGING_MEASURE_DELAY_MS);
    m_charging_state = nrf_gpio_pin_read(BAT_STAT_PIN);
    nrf_gpio_cfg_input(BAT_STAT_PIN, NRF_GPIO_PIN_NOPULL);

    m_on_measured = on_measured;
    m_on_charge_changed = on_charge_changed;

    bat_measure();

    return err_code;
}

ret_code_t battery_timer_start()
{
    ret_code_t err_code = app_timer_start(m_bat_timer, BAT_TIMER_INTERVAL, NULL);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = app_timer_start(m_bat_charging_state_timer, BAT_CHARGING_STATE_INTERVAL, NULL);

    return err_code;
}

uint8_t battery_get_charging_state()
{
    return !m_charging_state;
}

uint8_t battery_get_level()
{
    return m_bat_level;
}

bool battery_is_low_battery()
{
    return m_bat_level < 10;
}