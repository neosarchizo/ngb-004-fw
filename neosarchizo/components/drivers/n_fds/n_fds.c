#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"
#include "fds.h"

#include "n_fds.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"

static volatile bool m_initiated = false;

static void fds_evt_handler(fds_evt_t const *p_evt)
{
    switch (p_evt->id)
    {
    case FDS_EVT_INIT:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_INIT");
        if (p_evt->result == NRF_SUCCESS)
        {
            m_initiated = true;
        }
        break;
    }
    case FDS_EVT_UPDATE:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_UPDATE (%d)", p_evt->result);
        break;
    }
    case FDS_EVT_WRITE:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_WRITE (%d)", p_evt->result);
        break;
    }
    case FDS_EVT_DEL_RECORD:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_DEL_RECORD (%d)", p_evt->result);
        break;
    }
    case FDS_EVT_DEL_FILE:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_DEL_FILE (%d)", p_evt->result);
        break;
    }
    case FDS_EVT_GC:
    {
        NRF_LOG_INFO("fds_evt_handler : FDS_EVT_GC (%d)", p_evt->result);
        break;
    }
    default:
        break;
    }
}

ret_code_t n_fds_init()
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = fds_register(fds_evt_handler);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = fds_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    while (!m_initiated)
    {
    }

    err_code = n_fds_gc();

    return err_code;
}

ret_code_t n_fds_gc()
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_stat_t stat = {0};

    err_code = fds_stat(&stat);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (stat.corruption || stat.dirty_records > 0 || stat.freeable_words > 0)
    {
        err_code = fds_gc();
    }
 
    return err_code;
}

ret_code_t n_fds_write(uint16_t file_id, uint16_t key, uint8_t *p_data, uint32_t length)
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_record_desc_t desc = {0};
    fds_record_t record = {
        .file_id = file_id,
        .key = key,
        .data.p_data = p_data,
        .data.length_words = (length + 3) / sizeof(uint32_t),
    };

    err_code = fds_record_write(&desc, &record);

    return err_code;
}

ret_code_t n_fds_update(uint16_t file_id, uint16_t key, uint8_t *p_data, uint32_t length)
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    err_code = fds_record_find(file_id, key, &desc, &tok);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    fds_record_t record = {
        .file_id = file_id,
        .key = key,
        .data.p_data = p_data,
        .data.length_words = (length + 3) / sizeof(uint32_t),
    };

    err_code = fds_record_update(&desc, &record);

    return err_code;
}

ret_code_t n_fds_del(uint16_t file_id, uint16_t key)
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    err_code = fds_record_find(file_id, key, &desc, &tok);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = fds_record_delete(&desc);

    return err_code;
}

ret_code_t n_fds_find(uint16_t file_id, uint16_t key)
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    err_code = fds_record_find(file_id, key, &desc, &tok);

    return err_code;
}

ret_code_t n_fds_open(uint16_t file_id, uint16_t key, uint8_t *p_data, uint32_t length)
{
    ret_code_t err_code = NRF_SUCCESS;

    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    err_code = fds_record_find(file_id, key, &desc, &tok);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    fds_flash_record_t record = {0};

    err_code = fds_record_open(&desc, &record);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    memcpy(p_data, record.p_data, length);

    err_code = fds_record_close(&desc);

    return err_code;
}

ret_code_t n_fds_open_in_file(uint16_t file_id, uint16_t file_count, n_fds_cb_t on_found){
    ret_code_t err_code = NRF_SUCCESS;

    fds_flash_record_t flash_record;
    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    uint16_t found_count = 0;

    while (fds_record_find_in_file(file_id, &desc, &tok) == NRF_SUCCESS)
    {
        err_code = fds_record_open(&desc, &flash_record);
        if (err_code != NRF_SUCCESS)
        {
            break;
        }
        else
        {
            if (on_found != NULL)
            {
                on_found(flash_record.p_data);
            }
            
            ++found_count;
        }

        err_code = fds_record_close(&desc);
        if (err_code != NRF_SUCCESS)
        {
            break;
        }

        if (found_count == file_count)
            break;
    }

    return err_code;
}