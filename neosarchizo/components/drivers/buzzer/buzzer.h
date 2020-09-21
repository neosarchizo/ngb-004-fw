#ifndef BUZZER_H_
#define BUZZER_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BUZZER_PIN 18
#define BUZZER_MAXIMUM_TIME_MS 60000
#define BUZZER_DELAY_MS 1

    typedef struct
    {
        uint16_t hertz;
        uint16_t duration;
    } buzzer_note;

    extern ret_code_t buzzer_init();
    extern ret_code_t buzzer_buzz(uint32_t duration_ms);
    

    extern ret_code_t buzzer_play(buzzer_note * notes, uint8_t length, uint32_t duration_ms);
    extern ret_code_t buzzer_stop();

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H_ */