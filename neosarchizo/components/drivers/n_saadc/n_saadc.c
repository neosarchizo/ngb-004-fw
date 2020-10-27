#include "sdk_config.h"

#include "nordic_common.h"

#include "n_saadc.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_gpiote.h"

static nrf_saadc_value_t m_adc_buf[2];
static volatile bool m_measuring = false;

static n_saadc_cb_t m_on_measured = NULL;

static void oversample_timeout(void *p_context);

void saadc_event_handler(nrf_drv_saadc_evt_t const *p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        uint32_t err_code;

        nrf_saadc_value_t adc_result = p_event->data.done.p_buffer[0];

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);

        nrf_drv_saadc_uninit();
        if (m_on_measured != NULL)
        {
            m_on_measured(adc_result);
            m_on_measured = NULL;
        }
        
        m_measuring = false;
    }
}

ret_code_t n_saadc_measure(uint8_t pin, n_saadc_cb_t on_measured)
{
    ret_code_t err_code = NRF_SUCCESS;

    if (m_measuring)
    {
        err_code = NRF_ERROR_INTERNAL;
        return err_code;
    }

    m_measuring = true;

    err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(pin);

    err_code = nrf_drv_saadc_channel_init(0, &config);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = nrf_drv_saadc_buffer_convert(&m_adc_buf[0], 1);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = nrf_drv_saadc_buffer_convert(&m_adc_buf[1], 1);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = nrf_drv_saadc_sample();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    m_on_measured = on_measured;

    return err_code;
}