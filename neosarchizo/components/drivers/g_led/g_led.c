#include "sdk_config.h"

#include "nordic_common.h"

#include "g_led.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "alarm.h"
#include "battery.h"

static void g_led_turn_off()
{
    nrf_gpio_pin_write(G_LED_PIN_R, 1);
    nrf_gpio_pin_write(G_LED_PIN_G, 1);
    nrf_gpio_pin_write(G_LED_PIN_B, 1);
}

ret_code_t g_led_init()
{
    ret_code_t err_code = NRF_SUCCESS;

    nrf_gpio_cfg_output(G_LED_PIN_R);
    nrf_gpio_cfg_output(G_LED_PIN_G);
    nrf_gpio_cfg_output(G_LED_PIN_B);

    nrf_gpio_pin_write(G_LED_PIN_R, 1);
    nrf_gpio_pin_write(G_LED_PIN_G, 1);
    nrf_gpio_pin_write(G_LED_PIN_B, 1);

    return err_code;
}

void g_led_write(uint32_t pin_number, uint32_t value)
{
    nrf_gpio_pin_write(pin_number, !value);
}

void g_led_set_mode(uint8_t mode)
{
    switch (mode)
    {
    case G_LED_MODE_OFF:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 1);
        nrf_gpio_pin_write(G_LED_PIN_G, 1);
        nrf_gpio_pin_write(G_LED_PIN_B, 1);
        break;
    }
    case G_LED_MODE_BLUE:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 1);
        nrf_gpio_pin_write(G_LED_PIN_G, 1);
        nrf_gpio_pin_write(G_LED_PIN_B, 0);
        break;
    }
    case G_LED_MODE_YELLOW:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 0);
        nrf_gpio_pin_write(G_LED_PIN_G, 0);
        nrf_gpio_pin_write(G_LED_PIN_B, 1);
        break;
    }
    case G_LED_MODE_GREEN:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 1);
        nrf_gpio_pin_write(G_LED_PIN_G, 0);
        nrf_gpio_pin_write(G_LED_PIN_B, 1);
        break;
    }
    case G_LED_MODE_RED:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 0);
        nrf_gpio_pin_write(G_LED_PIN_G, 1);
        nrf_gpio_pin_write(G_LED_PIN_B, 1);
        break;
    }
    case G_LED_MODE_WHITE:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 0);
        nrf_gpio_pin_write(G_LED_PIN_G, 0);
        nrf_gpio_pin_write(G_LED_PIN_B, 0);
        break;
    }
    case G_LED_MODE_PURPLE:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 0);
        nrf_gpio_pin_write(G_LED_PIN_G, 1);
        nrf_gpio_pin_write(G_LED_PIN_B, 0);
        break;
    }
    default:
    {
        nrf_gpio_pin_write(G_LED_PIN_R, 1);
        nrf_gpio_pin_write(G_LED_PIN_G, 1);
        nrf_gpio_pin_write(G_LED_PIN_B, 1);
        break;
    }
    }
}

void g_led_play_boot_animation()
{
    nrf_gpio_pin_write(G_LED_PIN_R, 1);
    nrf_gpio_pin_write(G_LED_PIN_G, 1);
    nrf_gpio_pin_write(G_LED_PIN_B, 0);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(G_LED_PIN_R, 1);
    nrf_gpio_pin_write(G_LED_PIN_G, 0);
    nrf_gpio_pin_write(G_LED_PIN_B, 1);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(G_LED_PIN_R, 0);
    nrf_gpio_pin_write(G_LED_PIN_G, 1);
    nrf_gpio_pin_write(G_LED_PIN_B, 1);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(G_LED_PIN_R, 0);
    nrf_gpio_pin_write(G_LED_PIN_G, 0);
    nrf_gpio_pin_write(G_LED_PIN_B, 0);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(G_LED_PIN_R, 1);
    nrf_gpio_pin_write(G_LED_PIN_G, 1);
    nrf_gpio_pin_write(G_LED_PIN_B, 1);
}