#include "sdk_config.h"

#include "nordic_common.h"

#include "multiplexer.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"

ret_code_t multiplexer_init()
{
    ret_code_t err_code = NRF_SUCCESS;

    nrf_gpio_cfg_output(MULTIPLEXER_PIN_EN);
    nrf_gpio_cfg_output(MULTIPLEXER_PIN_S0);
    nrf_gpio_cfg_output(MULTIPLEXER_PIN_S1);
    nrf_gpio_cfg_output(MULTIPLEXER_PIN_S2);

    nrf_gpio_pin_write(MULTIPLEXER_PIN_EN, 0);
    nrf_gpio_pin_write(MULTIPLEXER_PIN_S0, 0);
    nrf_gpio_pin_write(MULTIPLEXER_PIN_S1, 0);
    nrf_gpio_pin_write(MULTIPLEXER_PIN_S2, 0);

    return err_code;
}

void multiplexer_set_enabled(bool enabled)
{
    if (enabled)
    {
        nrf_gpio_pin_write(MULTIPLEXER_PIN_EN, 1);
    }
    else
    {
        nrf_gpio_pin_write(MULTIPLEXER_PIN_EN, 0);
    }
}

void multiplexer_set_bit(uint8_t value)
{
    if (value >= MULTIPLEXER_VALUE_MAX)
    {
        return;
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        if ((1 << i) & value > 0)
        {
            nrf_gpio_pin_write(MULTIPLEXER_PIN_S0 + i, 1);
        } else {
            nrf_gpio_pin_write(MULTIPLEXER_PIN_S0 + i, 0);
        }
    }
}