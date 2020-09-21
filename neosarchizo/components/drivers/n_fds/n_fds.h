#ifndef N_FDS_H_
#define N_FDS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_util_platform.h"
#include "fds.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint32_t record_id;
        uint16_t file_id;
        uint16_t record_key;
        bool is_record_updated;
    } n_fds_write_data;

    typedef struct
    {
        uint32_t record_id;
        uint16_t file_id;
        uint16_t record_key;
        bool is_record_updated;
    } n_fds_del_data;

    typedef void (*n_fds_cb_t)(const uint8_t * p_data);

    extern ret_code_t n_fds_init();
    extern ret_code_t n_fds_gc();
    extern ret_code_t n_fds_write(uint16_t file_id, uint16_t key, uint8_t * p_data, uint32_t length);
    extern ret_code_t n_fds_update(uint16_t file_id, uint16_t key, uint8_t * p_data, uint32_t length);
    extern ret_code_t n_fds_del(uint16_t file_id, uint16_t key);
    extern ret_code_t n_fds_find(uint16_t file_id, uint16_t key);
    extern ret_code_t n_fds_open(uint16_t file_id, uint16_t key, uint8_t * p_data, uint32_t length);
    extern ret_code_t n_fds_open_in_file(uint16_t file_id, uint16_t file_count, n_fds_cb_t on_found);

#ifdef __cplusplus
}
#endif

#endif /* N_FDS_H_ */