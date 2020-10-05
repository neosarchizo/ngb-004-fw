#ifndef G_MOTOR_H_
#define G_MOTOR_H_

#include <stdint.h>
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define G_MOTOR_PIN 17

#define G_MOTOR_MODE_OFF 0
#define G_MOTOR_MODE_ON 1

    extern ret_code_t g_motor_init();
    extern void g_motor_set_mode(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* G_MOTOR_H_ */