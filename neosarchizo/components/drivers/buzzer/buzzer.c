#include "sdk_config.h"

#include "nordic_common.h"
#include "app_timer.h"

#include "buzzer.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_pwm.h"
#include "nrf_log.h"

APP_TIMER_DEF(m_buzzer_timer);

APP_PWM_INSTANCE(PWM0, 1);

static void pwm_ready_callback(uint32_t pwm_id);
static void pwm_off();

static volatile bool m_ready_flag = false;

static volatile bool m_pwm_on = false;

static volatile bool m_playing = false;

static buzzer_note *m_notes = NULL;
static volatile uint8_t m_notes_length = 0;
static volatile uint32_t m_notes_duration_ms = 0;
static volatile uint32_t m_cycle_duration_ms = 0;
static volatile uint32_t m_total_cycle_duration_ms = 0;
static volatile uint32_t m_total_duration_ms = 0;

static ret_code_t pwm_on(uint16_t hertz)
{
  ret_code_t err_code = NRF_SUCCESS;

  if (m_pwm_on)
  {
    return err_code;
  }

  m_pwm_on = true;

  m_ready_flag = false;

  app_pwm_config_t pwm0_cfg = APP_PWM_DEFAULT_CONFIG_1CH(hertz, 18);
  pwm0_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
  err_code = app_pwm_init(&PWM0, &pwm0_cfg, pwm_ready_callback);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }

  app_pwm_enable(&PWM0);

  err_code = app_pwm_channel_duty_set(&PWM0, 0, 50);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }

  return err_code;
}

static void pwm_off()
{
  if (!m_pwm_on)
  {
    return;
  }

  app_pwm_channel_duty_set(&PWM0, 0, 0);
  app_pwm_disable(&PWM0);
  app_pwm_uninit(&PWM0);

  m_pwm_on = false;
}

static void buzzer_timeout(void *p_context)
{
  UNUSED_PARAMETER(p_context);

  uint32_t position = 0;

  for (uint8_t i = 0; i < m_notes_length; i++)
  {
    position += m_notes[i].duration;
    if (m_cycle_duration_ms <= position)
    {
      if (m_notes[i].hertz > 0)
      {
        pwm_on(m_notes[i].hertz);
      }
      else
      {
        pwm_off();
      }

      break;
    }
  }

  ++m_cycle_duration_ms;
  ++m_total_duration_ms;

  if (m_cycle_duration_ms >= m_total_cycle_duration_ms)
  {
    m_cycle_duration_ms = 0;
  }
  
  if (m_total_duration_ms >= m_notes_duration_ms)
  {
    buzzer_stop();
  }
}

static void pwm_ready_callback(uint32_t pwm_id) // PWM callback function
{
  m_ready_flag = true;
}

ret_code_t buzzer_init()
{
  ret_code_t err_code = app_timer_create(&m_buzzer_timer,
                                         APP_TIMER_MODE_REPEATED,
                                         buzzer_timeout);
  m_ready_flag = false;

  return err_code;
}

ret_code_t buzzer_stop()
{
  ret_code_t err_code = NRF_SUCCESS;

  if (!m_playing)
  {
    return err_code;
  }

  err_code = app_timer_stop(m_buzzer_timer);
  pwm_off();
  m_playing = false;

  return err_code;
}

ret_code_t buzzer_play(buzzer_note *notes, uint8_t length, uint32_t duration_ms)
{
  ret_code_t err_code = NRF_SUCCESS;

  if (duration_ms > BUZZER_MAXIMUM_TIME_MS)
  {
    return NRF_ERROR_INTERNAL;
  }

  if (m_playing)
  {
    return NRF_ERROR_INTERNAL;
  }

  m_playing = true;

  m_notes = notes;
  m_notes_length = length;
  m_notes_duration_ms = duration_ms;
  m_cycle_duration_ms = 0;
  m_total_duration_ms = 0;

  m_total_cycle_duration_ms = 0;

  for (uint8_t i = 0; i < m_notes_length; i++) {
    m_total_cycle_duration_ms += m_notes[i].duration;
  }
  
  err_code = app_timer_start(m_buzzer_timer, APP_TIMER_TICKS(BUZZER_DELAY_MS), NULL);
  if (err_code != NRF_SUCCESS)
  {
    m_playing = false;
    return err_code;
  }

  return err_code;
}