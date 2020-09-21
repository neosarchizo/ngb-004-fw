#ifndef G_LED_H_
#define G_LED_H_

#include <stdint.h>
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define G_LED_PIN_R 24
#define G_LED_PIN_G 23
#define G_LED_PIN_B 22

#define G_LED_MODE_OFF 0   // NORMAL
#define G_LED_MODE_BLUE 1  // CONNECTED
#define G_LED_MODE_GREEN 2 // ADVERTISING
#define G_LED_MODE_RED 3   // LOW BATTERY, CHARGING (CONTINUOUS)
#define G_LED_MODE_WHITE 4 // ALARM
#define G_LED_MODE_YELLOW 5 // BUTTON PRESSED
#define G_LED_MODE_PURPLE 6 // BUTTON PRESSED

#define G_LED_LOW_BATTERY_COUNTER_MAX 3

    extern ret_code_t g_led_init();
    extern void g_led_write(uint32_t pin_number, uint32_t value);
    extern void g_led_set_mode(uint8_t mode);
    extern void g_led_play_boot_animation();

#ifdef __cplusplus
}
#endif

#endif /* G_LED_H_ */