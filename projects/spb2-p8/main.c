#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "ble_cus.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"

// Drivers
#include "qre1113gr.h"
#include "mlx90248.h"
#include "buzzer.h"
#include "g_button.h"
#include "g_led.h"
#include "g_motor.h"
#include "battery.h"
#include "n_fds.h"
#include "multiplexer.h"

// Libraries
#include "packet.h"
#include "n_time.h"
#include "g_log.h"
#include "alarm.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define DEVICE_NAME "GSN-SPB2P8"      /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME "GASIAN" /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL 950       /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION 0      /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO 3 /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG 1  /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL MSEC_TO_UNITS(100, UNIT_1_25_MS) /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(200, UNIT_1_25_MS) /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY 0                                    /**< Slave latency. */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)   /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT 3                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

// Button
#define BUTTON_PRESSED_DELAY_MS 200

// LED
#define LED_LONG_DELAY_MS 5000
#define LED_SHORT_DELAY_MS 100

// Overdose
#define OVERDOSE_DELAY_MS 1000
#define OVERDOSE_TIMEOUT_SEC 3600
#define OVERDOSE_TAKEN_TIMES_LENGTH 5
#define OVERDOSE_ALARM_DELAY_SEC 2
#define OVERDOSE_ALARM_BUZZER_DELAY_MS (OVERDOSE_ALARM_DELAY_SEC * 1000)
// Connected
#define CONNECTED_LED_ON_DELAY_MS 1500

// Disconnect
#define DISCONNECT_ALARM_DELAY_SEC 3
#define DISCONNECT_ALARM_BUZZER_DELAY_MS (DISCONNECT_ALARM_DELAY_SEC * 1000)

NRF_BLE_GATT_DEF(m_gatt);           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising); /**< Advertising module instance. */

BLE_CUS_DEF(m_cus);
APP_TIMER_DEF(m_led_timer);
APP_TIMER_DEF(m_button_timer);
APP_TIMER_DEF(m_connected_timer);
APP_TIMER_DEF(m_overdose_timer);

// Bluetooth
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
static ble_uuid_t m_adv_uuids[] =                        /**< Universally unique service identifiers. */
    {
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}};

static void advertising_start();
static void on_time_changed(n_time_data data);
static void on_minute_changed(n_time_data data);
static void on_day_changed(n_time_data data);

// Button
static volatile bool m_button_pressed = false;

// LED
static volatile bool m_led_toggle = false;

// Overdose
static uint16_t m_taken_times[OVERDOSE_TAKEN_TIMES_LENGTH - 1];
static volatile uint8_t m_overdosed_counter = 0;

// Connected
static volatile bool m_connected_led_on = false;

// Buzzer

#define BUZZER_NOTES_LENGTH 1

static buzzer_note m_buzzer_notes[BUZZER_NOTES_LENGTH] = {
    {
        .hertz = 2000,
        .duration = 1,
    },
};

void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static ret_code_t notify_response(uint8_t cmd, uint8_t char_id, bool success)
{
    ret_code_t err_code = NRF_SUCCESS;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t packet_length = 6;
        uint8_t packet_buffer[packet_length];

        packet_buffer[3] = !success;
        packet_generate_packet(cmd, packet_length, packet_buffer);

        err_code = ble_cus_char_write(&m_cus, char_id, packet_buffer, packet_length);

        if (err_code != NRF_SUCCESS)
        {
            NRF_LOG_INFO("notify_response failed : ble_cus_char_write");
            NRF_LOG_INFO("err_code : %X", err_code);
            return err_code;
        }
    }
    else
    {
        err_code = NRF_ERROR_BASE_NUM;
        NRF_LOG_INFO("notify_response failed : m_conn_handle is BLE_CONN_HANDLE_INVALID");
        return err_code;
    }

    return err_code;
}

static ret_code_t notify_data(uint8_t cmd, uint8_t char_id, uint8_t *data, uint8_t length)
{
    ret_code_t err_code = NRF_SUCCESS;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t packet_length = 5 + length;
        uint8_t packet_buffer[packet_length];

        for (uint8_t i = 0; i < length; i++)
        {
            packet_buffer[i + 3] = *(data + i);
        }

        packet_generate_packet(cmd, packet_length, packet_buffer);

        err_code = ble_cus_char_write(&m_cus, char_id, packet_buffer, packet_length);

        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }
    else
    {
        err_code = NRF_ERROR_BASE_NUM;
        return err_code;
    }

    return err_code;
}

static void led_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t err_code = NRF_SUCCESS;

    if (m_led_toggle)
    {
        // SHORT
        if (!m_connected_led_on && !m_button_pressed && m_overdosed_counter == 0)
        {
            if (alarm_alarming())
            {
                // Alarming
                // g_led_set_mode(G_LED_MODE_WHITE);
            }
            else if (battery_get_charging_state())
            {
                // Charging
                g_led_set_mode(G_LED_MODE_RED);
            }
            else if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                // Connected
                g_led_set_mode(G_LED_MODE_BLUE);
            }
            else if (battery_is_low_battery())
            {
                // low battery
                g_led_set_mode(G_LED_MODE_RED);
            }
            else
            {
                // Normal
                g_led_set_mode(G_LED_MODE_OFF);
            }
        }

        err_code = app_timer_start(m_led_timer, APP_TIMER_TICKS(LED_SHORT_DELAY_MS), NULL);
    }
    else
    {
        // LONG
        if (!m_connected_led_on && !m_button_pressed && m_overdosed_counter == 0)
        {
            if (alarm_alarming())
            {
                // Alarming
                // g_led_set_mode(G_LED_MODE_WHITE);
            }
            else if (battery_get_charging_state())
            {
                // Charging
                g_led_set_mode(G_LED_MODE_RED);
            }
            else if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                // Connected
                g_led_set_mode(G_LED_MODE_OFF);
            }
            else if (battery_is_low_battery())
            {
                // low battery
                g_led_set_mode(G_LED_MODE_OFF);
            }
            else
            {
                // Normal
                g_led_set_mode(G_LED_MODE_OFF);
            }
        }

        err_code = app_timer_start(m_led_timer, APP_TIMER_TICKS(LED_LONG_DELAY_MS), NULL);
    }

    m_led_toggle = !m_led_toggle;
}

static void button_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    g_led_set_mode(G_LED_MODE_OFF);
    m_button_pressed = false;
}

static void connected_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    g_led_set_mode(G_LED_MODE_OFF);
    m_connected_led_on = false;
}

static void overdose_timeout(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    for (uint8_t i = 0; i < OVERDOSE_TAKEN_TIMES_LENGTH - 1; i++)
    {
        if (m_taken_times[i] > 0)
        {
            --m_taken_times[i];
        }
    }

    if (m_overdosed_counter > 0)
    {
        if (--m_overdosed_counter == 0)
        {
            g_led_set_mode(G_LED_MODE_OFF);
        }
    }
}

static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_led_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                led_timeout);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_button_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                button_timeout);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_connected_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                connected_timeout);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_overdose_timer,
                                APP_TIMER_MODE_REPEATED,
                                overdose_timeout);
    APP_ERROR_CHECK(err_code);

    err_code = n_time_init(on_time_changed, on_minute_changed, on_day_changed);
    APP_ERROR_CHECK(err_code);

    err_code = buzzer_init();
    APP_ERROR_CHECK(err_code);
}

static void gap_params_init(void)
{
    ret_code_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void char_cmd_write_callback(uint8_t *data, uint8_t length)
{
    ret_code_t err_code;

    if (packet_check(length, data))
    {
        uint8_t cmd = packet_get_cmd(data);
        uint8_t length = packet_get_length(data);
        uint8_t buffer[length];
        uint8_t tx_data[20]; // maximum size : 2

        packet_get_data(data, length, buffer);
        NRF_LOG_INFO("char_cmd_write_callback : %d", cmd);

        switch (cmd)
        {
        case PACKET_CMD_IR:
        {
            NRF_LOG_INFO("PACKET_CMD_IR");
            if (length != 0)
            {
                notify_response(PACKET_CMD_IR, 0, false);
                return;
            }

            if (!mlx90248_lid_closed())
            {
                qre1113gr_update();
                return;
            }

            qre1113gr_data ir_data;

            qre1113gr_read(&ir_data);

            tx_data[0] = ir_data.a;
            tx_data[1] = ir_data.b;
            tx_data[2] = ir_data.c;
            tx_data[3] = ir_data.d;
            notify_data(PACKET_CMD_IR, 0, tx_data, 4);
            break;
        }
        case PACKET_CMD_HALL:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_HALL, 0, false);
                return;
            }

            mlx90248_data hall_data;
            mlx90248_read(&hall_data);

            tx_data[0] = hall_data.bowl;
            tx_data[1] = hall_data.lid;
            notify_data(PACKET_CMD_HALL, 0, tx_data, 2);
            break;
        }
        case PACKET_CMD_BUTTON:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_BUTTON, 0, false);
                return;
            }

            g_button_read(&tx_data[0]);
            notify_data(PACKET_CMD_BUTTON, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_LED:
        {
            if (length != 3)
            {
                notify_response(PACKET_CMD_LED, 0, false);
                return;
            }

            g_led_write(G_LED_PIN_R, buffer[0]);
            g_led_write(G_LED_PIN_G, buffer[1]);
            g_led_write(G_LED_PIN_B, buffer[2]);

            notify_response(PACKET_CMD_LED, 0, true);
            break;
        }
        case PACKET_CMD_BUZZER:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_BUZZER, 0, false);
                return;
            }

            buzzer_play(m_buzzer_notes, BUZZER_NOTES_LENGTH, 100);

            notify_response(PACKET_CMD_BUZZER, 0, true);
            break;
        }
        case PACKET_CMD_SET_N_TIME:
        {
            if (length != 6)
            {
                notify_response(PACKET_CMD_SET_N_TIME, 0, false);
                return;
            }

            tx_data[0] = n_time_set((n_time_data *)buffer);
            notify_data(PACKET_CMD_SET_N_TIME, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_BATTERY:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_BATTERY, 0, false);
                return;
            }

            tx_data[0] = battery_get_level();
            notify_data(PACKET_CMD_BATTERY, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_CHARGING:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_CHARGING, 0, false);
                return;
            }

            tx_data[0] = battery_get_charging_state();
            notify_data(PACKET_CMD_CHARGING, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_GET_LOGS:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_GET_LOGS, 0, false);
                return;
            }

            g_log_get_data();
            break;
        }
        case PACKET_CMD_CLEAR_LOGS:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_CLEAR_LOGS, 0, false);
                return;
            }

            tx_data[0] = g_log_clear();
            notify_data(PACKET_CMD_CLEAR_LOGS, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_CLEAR_ALARMS:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_CLEAR_ALARMS, 0, false);
                return;
            }

            tx_data[0] = alarm_clear();
            notify_data(PACKET_CMD_CLEAR_ALARMS, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_ADD_ALARM:
        {
            NRF_LOG_INFO("PACKET_CMD_ADD_ALARM");

            if (length != 7)
            {
                notify_response(PACKET_CMD_ADD_ALARM, 0, false);
                NRF_LOG_INFO("PACKET_CMD_ADD_ALARM : failed");
                return;
            }

            tx_data[0] = alarm_add((alarm_time *)buffer);
            notify_data(PACKET_CMD_ADD_ALARM, 0, tx_data, 1);

            NRF_LOG_HEXDUMP_INFO(buffer, 7);

            NRF_LOG_INFO("PACKET_CMD_ADD_ALARM : success");
            break;
        }
        case PACKET_CMD_SET_ALARM_MUTED:
        {
            if (length != 1)
            {
                notify_response(PACKET_CMD_SET_ALARM_MUTED, 0, false);
                return;
            }

            alarm_set_muted(buffer[0]);
            tx_data[0] = alarm_muted();
            notify_data(PACKET_CMD_SET_ALARM_MUTED, 0, tx_data, 1);
            break;
        }
        case PACKET_CMD_TRIG_ALARM:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_TRIG_ALARM, 0, false);
                return;
            }

            alarm_trig(false, 0, 0);
            notify_response(PACKET_CMD_TRIG_ALARM, 0, true);
            break;
        }
        default:
            break;
        }
    }
}

static void cus_init(void)
{
    ret_code_t err_code;
    err_code = ble_cus_init(&m_cus);
    APP_ERROR_CHECK(err_code);

    ble_cus_set_callback_char_write(0, char_cmd_write_callback);
}

static void services_init(void)
{
    ret_code_t err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    cus_init();
}

static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail = false;
    cp_init.evt_handler = on_conn_params_evt;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

static void application_timers_start(void)
{
    ret_code_t err_code;

    err_code = app_timer_start(m_led_timer, APP_TIMER_TICKS(LED_LONG_DELAY_MS), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_overdose_timer, APP_TIMER_TICKS(OVERDOSE_DELAY_MS), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = n_time_timer_start();
    APP_ERROR_CHECK(err_code);

    err_code = battery_timer_start();
    APP_ERROR_CHECK(err_code);

    err_code = mlx90248_timer_start();
    APP_ERROR_CHECK(err_code);
}

static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        NRF_LOG_INFO("Fast advertising.");
        break;
    case BLE_ADV_EVT_SLOW:
        NRF_LOG_INFO("Slow advertising.");
        break;
    case BLE_ADV_EVT_IDLE:
        break;
    default:
        break;
    }
}

static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected.");
        m_conn_handle = BLE_CONN_HANDLE_INVALID;
        break;

    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected.");
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);

        m_connected_led_on = true;
        g_led_set_mode(G_LED_MODE_GREEN);
        err_code = app_timer_start(m_connected_timer, APP_TIMER_TICKS(CONNECTED_LED_ON_DELAY_MS), NULL);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        NRF_LOG_DEBUG("PHY update request.");
        ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
    }
    break;

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRF_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRF_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    default:
        // No implementation needed.
        break;
    }
}

static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

static void advertising_init(void)
{
    ret_code_t err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids = m_adv_uuids;

    init.config.ble_adv_fast_enabled = false;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout = 0;

    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_slow_timeout = 0;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

static void advertising_start()
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

// callbacks

static void on_time_changed(n_time_data data)
{
    NRF_LOG_INFO("%d (%d) %02d-%02d-%02d", data.current_days, data.day_of_the_week, data.hours, data.minutes, data.seconds);
}

static void on_minute_changed(n_time_data data)
{
    qre1113gr_data ir_data;
    qre1113gr_read(&ir_data);

    alarm_check(data, ir_data);
}

static void on_day_changed(n_time_data data)
{
    n_fds_gc();
}

static void on_bat_measured(uint8_t value)
{
    notify_data(PACKET_CMD_BATTERY, 0, &value, 1);

    if (value < 1)
    {
        nrf_gpio_cfg_input(BAT_STAT_PIN, NRF_GPIO_PIN_PULLUP);
        nrf_gpio_cfg_sense_set(BAT_STAT_PIN, NRF_GPIO_PIN_SENSE_LOW);
        g_led_set_mode(G_LED_MODE_OFF);
        // nrf_gpio_cfg_sense_set(G_BUTTON_PIN, NRF_GPIO_PIN_SENSE_LOW);
        // g_led_set_mode(G_LED_MODE_OFF);

        sd_power_system_off();
    }
}

static void on_charge_changed(uint8_t value)
{
    notify_data(PACKET_CMD_CHARGING, 0, &value, 1);

    if (value)
    {
        g_led_set_mode(G_LED_MODE_RED);
    }
    else
    {
        g_led_set_mode(G_LED_MODE_OFF);
    }
}

static void on_button_pressed()
{
    ret_code_t err_code = NRF_SUCCESS;

    uint8_t pressed = 1;
    notify_data(PACKET_CMD_BUTTON, 0, &pressed, 1);

    if (alarm_alarming())
    {
        alarm_cancel();
        g_led_set_mode(G_LED_MODE_OFF);
    }
    else
    {
        m_button_pressed = true;
        g_led_set_mode(G_LED_MODE_YELLOW);
        buzzer_play(m_buzzer_notes, BUZZER_NOTES_LENGTH, BUTTON_PRESSED_DELAY_MS);
        err_code = app_timer_stop(m_button_timer);
        err_code = app_timer_start(m_button_timer, APP_TIMER_TICKS(BUTTON_PRESSED_DELAY_MS), NULL);
    }
}

static void on_button_long_pressed()
{
    NRF_LOG_INFO("on_button_long_pressed");
    ret_code_t err_code = NRF_SUCCESS;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        // TODO
        err_code = sd_ble_gap_disconnect(m_conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
            g_led_set_mode(G_LED_MODE_PURPLE);
            buzzer_play(m_buzzer_notes, BUZZER_NOTES_LENGTH, DISCONNECT_ALARM_BUZZER_DELAY_MS);
        }
    }
}

static void on_hall_changed(mlx90248_data data)
{
    uint8_t tx_data[2];
    tx_data[0] = data.bowl;
    tx_data[1] = data.lid;
    notify_data(PACKET_CMD_HALL, 0, tx_data, 2);
}

static void on_ir_measured(qre1113gr_data data)
{
    NRF_LOG_INFO("on_ir_measured");
    uint8_t tx_data[4];

    tx_data[0] = data.a;
    tx_data[1] = data.b;
    tx_data[2] = data.c;
    tx_data[3] = data.d;
    notify_data(PACKET_CMD_IR, 0, tx_data, 4);
}

static void check_overdose()
{
    bool is_all_taken = true;

    for (size_t i = 0; i < OVERDOSE_TAKEN_TIMES_LENGTH - 1; i++)
    {
        if (!m_taken_times[i])
        {
            is_all_taken = false;
        }
    }

    if (is_all_taken)
    {
        // overdose!
        m_overdosed_counter = OVERDOSE_ALARM_DELAY_SEC;
        g_led_set_mode(G_LED_MODE_RED);
        buzzer_play(m_buzzer_notes, BUZZER_NOTES_LENGTH, OVERDOSE_ALARM_BUZZER_DELAY_MS);

        notify_response(PACKET_CMD_OVERDOSE_ALARM, 0, true);

        memset(m_taken_times, 0, sizeof(m_taken_times));
    }
    else
    {
        for (size_t i = 0; i < OVERDOSE_TAKEN_TIMES_LENGTH - 1; i++)
        {
            if (!m_taken_times[i])
            {
                m_taken_times[i] = OVERDOSE_TIMEOUT_SEC;
                break;
            }
        }
    }
}

static void on_lid_closed(mlx90248_data data)
{
    if (g_log_initiated() && m_conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        g_log_add();
    }

    qre1113gr_update();

    check_overdose();
}

static void on_lid_opened(mlx90248_data data)
{
    if (alarm_alarming())
    {
        alarm_cancel();
    }
}

static void on_log_read(g_log_time data)
{
    uint8_t tx_data[sizeof(g_log_time)];

    memcpy(tx_data, &data, sizeof(g_log_time));
    notify_data(PACKET_CMD_GET_LOGS, 0, tx_data, sizeof(g_log_time));
}

static void on_alarm()
{
    // g_led_set_mode(G_LED_MODE_WHITE);
}

static void gpio_init(void)
{
    ret_code_t err_code;

    nrf_drv_gpiote_init();

    err_code = qre1113gr_init(on_ir_measured);
    APP_ERROR_CHECK(err_code);

    err_code = mlx90248_init(on_hall_changed, on_lid_closed, on_lid_opened);
    APP_ERROR_CHECK(err_code);

    err_code = g_button_init(on_button_pressed, on_button_long_pressed);
    APP_ERROR_CHECK(err_code);

    // call these functions after timers_init
    err_code = battery_init(on_bat_measured, on_charge_changed);
    APP_ERROR_CHECK(err_code);

    err_code = g_led_init();
    APP_ERROR_CHECK(err_code);

    err_code = g_motor_init();
    APP_ERROR_CHECK(err_code);

    err_code = multiplexer_init();
    APP_ERROR_CHECK(err_code);

    // g_motor_set_mode(G_MOTOR_MODE_ON);
    g_motor_set_mode(G_MOTOR_MODE_OFF);
}

static void my_fds_init(void)
{
    ret_code_t err_code;

    err_code = n_fds_init();
    APP_ERROR_CHECK(err_code);

    err_code = g_log_init(on_log_read);
    APP_ERROR_CHECK(err_code);

    err_code = alarm_init(on_alarm);
    APP_ERROR_CHECK(err_code);
}

int main(void)
{
    ret_code_t err_code;

    // Initialize.
    log_init();

    timers_init();

    power_management_init();

    gpio_init();

    ble_stack_init();
    gap_params_init();
    gatt_init();

    advertising_init();
    services_init();
    conn_params_init();

    my_fds_init();

    g_led_play_boot_animation();

    // Start execution.
    NRF_LOG_INFO("Gasian Smart Pillbox Start");

    application_timers_start();

    advertising_start();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}

/**
 * @}
 */
