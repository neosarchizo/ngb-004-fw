#ifndef G_BUTTON_H_
#define G_BUTTON_H_

#include <stdint.h>
#include "app_util_platform.h"
#include "nrfx_gpiote.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define G_BUTTON_PIN 13
#define G_BUTTON_LONG_PRESSED_INTERVAL APP_TIMER_TICKS(15000)

    typedef void (*g_button_cb_t)(void);

    extern ret_code_t g_button_init(g_button_cb_t on_pressed, g_button_cb_t on_long_pressed);
    extern void g_button_read(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* G_BUTTON_H_ */