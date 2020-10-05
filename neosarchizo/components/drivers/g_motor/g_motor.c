#include "sdk_config.h"

#include "nordic_common.h"

#include "g_motor.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"

ret_code_t g_motor_init()
{
    ret_code_t err_code = NRF_SUCCESS;
    nrf_gpio_cfg_output(G_MOTOR_PIN);
    nrf_gpio_pin_write(G_MOTOR_PIN, 0);
    return err_code;
}

void g_motor_set_mode(uint8_t mode)
{
    switch (mode)
    {
    case G_MOTOR_MODE_OFF:
    {
        nrf_gpio_pin_write(G_MOTOR_PIN, 0);
        break;
    }
    case G_MOTOR_MODE_ON:
    {
        nrf_gpio_pin_write(G_MOTOR_PIN, 1);
        break;
    }
    default:
    {
        break;
    }
    }
}
