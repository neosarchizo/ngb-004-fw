#ifndef MLX90248_H_
#define MLX90248_H_

#include <stdint.h>
#include "app_util_platform.h"
#include "nrfx_gpiote.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MLX90248_PIN_BOWL 8
#define MLX90248_PIN_LID 28
#define MLX90248_INTERVAL APP_TIMER_TICKS(40)

    typedef struct
    {
        uint8_t bowl;
        uint8_t lid;
    } mlx90248_data;

    typedef void (*mlx90248_cb_t)(mlx90248_data data);

    extern ret_code_t mlx90248_init(mlx90248_cb_t on_hall_changed, mlx90248_cb_t on_lid_closed, mlx90248_cb_t on_lid_opened);
    extern void mlx90248_read(mlx90248_data *data);
    extern bool mlx90248_lid_closed();
    extern ret_code_t mlx90248_timer_start();

#ifdef __cplusplus
}
#endif

#endif /* MLX90248_H_ */