#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_util_platform.h"
#include "nrf_drv_saadc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS 600  /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION 6     /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS 270 /**< Typical forward voltage drop of the diode . */
#define VOLTAGE_DIVIDER_SCALING_COMPENSATION 1.3918571429
#define ADC_RES_14BIT 16384 /**< Maximum digital value for 10-bit ADC conversion. */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
    ((((ADC_VALUE)*ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_14BIT) * ADC_PRE_SCALING_COMPENSATION)

#define BAT_VOLTAGE_MV_MAXIMUM 4158
#define BAT_VOLTAGE_MV_MINIMUM 3073
#define BAT_CHARGING_ON 0
#define BAT_CHARGING_OFF 1
#define BAT_ADC_PIN NRF_SAADC_INPUT_AIN7
#define BAT_STAT_PIN 7

#define BAT_TIMER_INTERVAL APP_TIMER_TICKS(120000) // APP_TIMER_TICKS(120000)
#define BAT_UNCHARGED_INTERVAL APP_TIMER_TICKS(5000)
#define BAT_OVERSAMPLE_INTERVAL APP_TIMER_TICKS(100)
#define BAT_CHARGING_STATE_INTERVAL APP_TIMER_TICKS(1000)
#define BAT_CHARGING_MEASURE_DELAY_MS 10
#define BAT_CHARGING_STATE_MEASURE_INTERVAL APP_TIMER_TICKS(BAT_CHARGING_MEASURE_DELAY_MS)

#define BAT_OVERSAMPLE 16

    typedef void (*bat_cb_t)(uint8_t value);
    
    extern ret_code_t battery_init(bat_cb_t on_measured, bat_cb_t on_charge_changed);
    extern ret_code_t battery_timer_start();
    extern uint8_t battery_get_charging_state();
    extern uint8_t battery_get_level();
    extern bool battery_is_low_battery();

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H_ */