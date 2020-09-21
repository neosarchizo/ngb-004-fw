#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "g_button.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"

APP_TIMER_DEF(m_long_pressed_timer);

static volatile bool m_long_pressed = false;
static volatile uint8_t m_state = 0;
static g_button_cb_t m_on_pressed = NULL;
static g_button_cb_t m_on_long_pressed = NULL;

static void long_pressed_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_on_long_pressed != NULL)
    {
        m_on_long_pressed();
    }

    m_long_pressed = true;
    ret_code_t err_code = NRF_SUCCESS;
}

static void g_button_in_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    uint8_t state = 0;

    switch (pin)
    {
    case G_BUTTON_PIN:
    {
        g_button_read(&state);

        if (m_state == 0 && state == 1)
        {
            m_long_pressed = false;

            if (m_on_pressed != NULL)
            {
                m_on_pressed();
            }

            app_timer_start(m_long_pressed_timer, G_BUTTON_LONG_PRESSED_INTERVAL, NULL);
        }
        else if (m_state == 1 && state == 0)
        {
            if (!m_long_pressed)
            {
                app_timer_stop(m_long_pressed_timer);
            }
        }
        m_state = state;
        break;
    }
    default:
        break;
    }
}

ret_code_t g_button_init(g_button_cb_t on_pressed, g_button_cb_t on_long_pressed)
{
    ret_code_t err_code;

    err_code = app_timer_create(&m_long_pressed_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                long_pressed_timeout);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    nrf_gpio_cfg_input(G_BUTTON_PIN, NRF_GPIO_PIN_PULLUP);

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(G_BUTTON_PIN, &in_config, g_button_in_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    nrf_drv_gpiote_in_event_enable(G_BUTTON_PIN, true);

    m_on_pressed = on_pressed;
    m_on_long_pressed = on_long_pressed;

    g_button_read(&m_state);

    return err_code;
}

void g_button_read(uint8_t *data)
{
    *data = !nrf_gpio_pin_read(G_BUTTON_PIN);
}