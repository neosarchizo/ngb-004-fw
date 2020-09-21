#include "sdk_config.h"
#include "nordic_common.h"
#include "app_timer.h"

#include "mlx90248.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_delay.h"

APP_TIMER_DEF(m_mlx90248_timer);

static mlx90248_data m_hall_data;
static mlx90248_cb_t m_on_hall_changed = NULL;
static mlx90248_cb_t m_on_lid_closed = NULL;
static mlx90248_cb_t m_on_lid_opened = NULL;

static void mlx90248_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    mlx90248_data hall_data;

    // nrf_gpio_cfg_input(MLX90248_PIN_BOWL, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(MLX90248_PIN_LID, NRF_GPIO_PIN_NOPULL);

    nrf_delay_us(50);
    
    // hall_data.bowl = nrf_gpio_pin_read(MLX90248_PIN_BOWL);
    hall_data.bowl = 0;
    hall_data.lid = nrf_gpio_pin_read(MLX90248_PIN_LID);

    nrf_gpio_pin_write(MLX90248_PIN_LID, 0);
    // nrf_gpio_pin_write(MLX90248_PIN_BOWL, 0);

    if ((m_hall_data.bowl != hall_data.bowl) || (m_hall_data.lid != hall_data.lid))
    {
        if (m_on_hall_changed != NULL)
        {
            m_on_hall_changed(hall_data);
        }

        if (m_hall_data.lid == 1 && hall_data.lid == 0)
        {
            if (m_on_lid_closed != NULL)
            {
                m_on_lid_closed(hall_data);
            }
        }

        if (m_hall_data.lid == 0 && hall_data.lid == 1)
        {
            if (m_on_lid_opened != NULL)
            {
                m_on_lid_opened(hall_data);
            }
        }
    }

    memcpy(&m_hall_data, &hall_data, sizeof(mlx90248_data));
}

ret_code_t mlx90248_init(mlx90248_cb_t on_hall_changed, mlx90248_cb_t on_lid_closed, mlx90248_cb_t on_lid_opened)
{
    ret_code_t err_code = app_timer_create(&m_mlx90248_timer,
                                           APP_TIMER_MODE_REPEATED,
                                           mlx90248_timeout);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // nrf_gpio_cfg_input(MLX90248_PIN_BOWL, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(MLX90248_PIN_LID, NRF_GPIO_PIN_NOPULL);

    nrf_delay_us(50);
    
    m_hall_data.bowl = 0;
    // m_hall_data.bowl = nrf_gpio_pin_read(MLX90248_PIN_BOWL);
    m_hall_data.lid = nrf_gpio_pin_read(MLX90248_PIN_LID);

    nrf_gpio_pin_write(MLX90248_PIN_LID, 0);
    // nrf_gpio_pin_write(MLX90248_PIN_BOWL, 0);
    
    m_on_hall_changed = on_hall_changed;
    m_on_lid_closed = on_lid_closed;
    m_on_lid_opened = on_lid_opened;

    return err_code;
}

ret_code_t mlx90248_timer_start()
{
    return app_timer_start(m_mlx90248_timer, MLX90248_INTERVAL, NULL);
}

bool mlx90248_lid_closed()
{
    return !m_hall_data.lid;
}

void mlx90248_read(mlx90248_data *data)
{
    memcpy(data, &m_hall_data, sizeof(mlx90248_data));
}