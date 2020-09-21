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
#include "fds.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "ble_cus.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "qre1113gr.h"
#include "mlx90248.h"
#include "buzzer.h"
#include "g_button.h"
#include "g_led.h"
#include "packet.h"
#include "nrf_drv_saadc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define DEVICE_NAME "GASIAN Health" /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME "GASIAN"  /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL 300        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

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

#define GPIO_INTERVAL APP_TIMER_TICKS(100)

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS 600            /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION 6               /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS 270           /**< Typical forward voltage drop of the diode . */
#define VOLTAGE_DIVIDER_SCALING_COMPENSATION 11 // 1.3918571429
// #define VOLTAGE_DIVIDER_SCALING_COMPENSATION 1.3928f // 1.3918571429
#define ADC_RES_14BIT 16384                          /**< Maximum digital value for 10-bit ADC conversion. */

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
    ((((ADC_VALUE)*ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_14BIT) * ADC_PRE_SCALING_COMPENSATION)

#define BAT_VOLTAGE_MV_MAXIMUM 3969
#define BAT_VOLTAGE_MV_MINIMUM 2448
#define BAT_CHARGING_ON 0
#define BAT_CHARGING_OFF 1

#define BATTERY_LEVEL_MEAS_INTERVAL APP_TIMER_TICKS(5000) //APP_TIMER_TICKS(120000)

APP_TIMER_DEF(m_gpio_timer_id);
APP_TIMER_DEF(m_battery_timer_id);

NRF_BLE_GATT_DEF(m_gatt);           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising); /**< Advertising module instance. */

BLE_CUS_DEF(m_cus);

static volatile uint8_t m_g_button_state = 0;
static mlx90248_data m_hall_data;

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

/* YOUR_JOB: Declare all services structure your application is using
 *  BLE_XYZ_DEF(m_xyz);
 */

// YOUR_JOB: Use UUIDs for service(s) used in your application.
static ble_uuid_t m_adv_uuids[] = /**< Universally unique service identifiers. */
    {
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}};

static nrf_saadc_value_t adc_buf[2];

static volatile uint8_t percentage_batt_lvl = 0;

static void advertising_start();

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for handling the ADC interrupt.
 *
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const *p_event)
{
    NRF_LOG_INFO("saadc_event_handler");
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrf_saadc_value_t adc_result;
        uint16_t batt_lvl_in_milli_volts;
        uint32_t err_code;

        adc_result = p_event->data.done.p_buffer[0];

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) * VOLTAGE_DIVIDER_SCALING_COMPENSATION;

        NRF_LOG_INFO("mV : %d", batt_lvl_in_milli_volts);

        if (batt_lvl_in_milli_volts >= BAT_VOLTAGE_MV_MAXIMUM)
        {
            percentage_batt_lvl = 100;
        }
        else if (batt_lvl_in_milli_volts <= BAT_VOLTAGE_MV_MINIMUM)
        {
            percentage_batt_lvl = 0;
        }
        else
        {
            percentage_batt_lvl = 100 * (((float)batt_lvl_in_milli_volts - BAT_VOLTAGE_MV_MINIMUM) / (BAT_VOLTAGE_MV_MAXIMUM - BAT_VOLTAGE_MV_MINIMUM));
        }

        NRF_LOG_INFO("battery : %d %", percentage_batt_lvl);

        // err_code = ble_bas_battery_level_update(&m_bas, percentage_batt_lvl, BLE_CONN_HANDLE_ALL);
        // if ((err_code != NRF_SUCCESS) &&
        //     (err_code != NRF_ERROR_INVALID_STATE) &&
        //     (err_code != NRF_ERROR_RESOURCES) &&
        //     (err_code != NRF_ERROR_BUSY) &&
        //     (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
        // {
        //     APP_ERROR_HANDLER(err_code);
        // }
    }
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
            NRF_LOG_INFO("notify_data failed : ble_cus_char_write");
            NRF_LOG_INFO("err_code : %X", err_code);
            return err_code;
        }
    }
    else
    {
        err_code = NRF_ERROR_BASE_NUM;
        NRF_LOG_INFO("notify_data failed : m_conn_handle is BLE_CONN_HANDLE_INVALID");
        return err_code;
    }

    return err_code;
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN1);

    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
}

static void battery_level_meas_timeout_handler(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    NRF_LOG_INFO("here!!");

    ret_code_t err_code;
    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

static void gpio_timeout_handler(void *p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code;

    qre1113gr_data ir_data;
    qre1113gr_read(&ir_data);

    mlx90248_data hall_data;
    mlx90248_read(&hall_data);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_gpio_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                gpio_timeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
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

    /* YOUR_JOB: Use an appearance value matching the application's use case.
       err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
       APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
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
        uint8_t tx_data[6]; // maximum size : 2

        packet_get_data(data, length, buffer);
        switch (cmd)
        {
        case PACKET_CMD_IR:
        {
            if (length != 0)
            {
                notify_response(PACKET_CMD_IR, 0, false);
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

            buzzer_buzz(10);

            notify_response(PACKET_CMD_BUZZER, 0, true);
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

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
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

/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    ret_code_t err_code;
    err_code = app_timer_start(m_gpio_timer_id, GPIO_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        NRF_LOG_INFO("Fast advertising.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_IDLE:
        sleep_mode_enter();
        break;

    default:
        break;
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected.");
        // LED indication will be changed when advertising starts.
        break;

    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected.");
        err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
        APP_ERROR_CHECK(err_code);
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
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

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for initializing the Advertising functionality.
 */
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

    init.config.ble_adv_fast_enabled = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

/**@brief Function for starting advertising.
 */
static void advertising_start()
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    //    NRF_LOG_INFO("in_pin_handler : (PIN) %d / (ACTION) %d", pin, action);

    uint8_t state = 0;
    uint8_t tx_data[6]; // maximum size : 2

    switch (pin)
    {
    case G_BUTTON_PIN:
    {
        g_button_read(&state);

        if (m_g_button_state == 0 && state == 1)
        {
//            tx_data[0] = state;
//            notify_data(PACKET_CMD_BUTTON, 0, tx_data, 1);
        }
        m_g_button_state = state;
        break;
    }
    case MLX90248_PIN_BOWL:
    case MLX90248_PIN_LID:
    {
        mlx90248_data hall_data;
        mlx90248_read(&hall_data);

        if ((m_hall_data.bowl != hall_data.bowl) || (m_hall_data.lid != hall_data.lid))
        {
//            tx_data[0] = hall_data.bowl;
//            tx_data[1] = hall_data.lid;
//            notify_data(PACKET_CMD_HALL, 0, tx_data, 2);
        }

        memcpy(&m_hall_data, &hall_data, sizeof(mlx90248_data));
        break;
    }
    default:
        break;
    }
}

static void gpio_init(void)
{
    ret_code_t err_code;

    nrf_drv_gpiote_init();

    qre1113gr_init();
    mlx90248_init(in_pin_handler);
    mlx90248_read(&m_hall_data);

    // buzzer_init();
    err_code = g_button_init(in_pin_handler);
    APP_ERROR_CHECK(err_code);
    uint8_t state = 0;
    g_button_read(&state);
    m_g_button_state = state;

    g_led_init();
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    log_init();
    timers_init();
    power_management_init();

    gpio_init();
    adc_configure();

//    ble_stack_init();
//    gap_params_init();
//    gatt_init();
//    advertising_init();
//    services_init();
//    conn_params_init();

    buzzer_init();

    g_led_write(G_LED_PIN_R, 1);
    nrf_delay_ms(100);
    g_led_write(G_LED_PIN_G, 1);
    nrf_delay_ms(100);
    g_led_write(G_LED_PIN_B, 1);
    nrf_delay_ms(100);

    // Start execution.
    NRF_LOG_INFO("Gasian Health Start");
    application_timers_start();

//    advertising_start();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}

/**
 * @}
 */
