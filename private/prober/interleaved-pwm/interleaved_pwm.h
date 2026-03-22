#ifndef PROBE_MANAGER_H
#define PROBE_MANAGER_H
#include <stdint.h>
//#include "pwm_line.h"


#define         ERR_PROBE_MANAGER_BASE                          0
#define         ERR_PROBE_MANAGER_INVALID_MEM                   (ERR_PROBE_MANAGER_BASE-1)
#define         ERR_PROBE_MANAGER_WRONG_PARAMETERS              (ERR_PROBE_MANAGER_BASE-2)
#define         ERR_PROBE_MANAGER_DEAD_TIME_LARGE               (ERR_PROBE_MANAGER_BASE-3)
#define         ERR_PROBE_MANAGER_FREQUENCY_LARGE               (ERR_PROBE_MANAGER_BASE-4)




/* Internal null-check helper — logs file+line and evaluates to true if NULL */
#define _PWM_NULL_CHECK(h) \
    (ESP_ERROR_CHECK_WITHOUT_ABORT((h) != NULL ? ESP_OK : ESP_ERR_INVALID_ARG), \
     (h) == NULL)

/**
 * @brief Start the interleaved PWM output on all channels.
 *
 * Begins switching all configured GPIO lines with the phase offsets
 * and pulse widths set at creation time.
 *
 * @param pwm  Pointer to an initialised interleaved_pwm_interface_t instance.
 
 * @return     0 on success, ESP_ERR_INVALID_ARG if pwm is NULL,
 *             non-zero on peripheral failure.
 *
 * @example
 *   interleaved_pwm_t pwm;
 *   interleavedPWMCreate(&pwm, &config);
 *   PWM_START(pwm);
 */
#define PWM_START(h) ({                                     \
    int _ret;                                               \
    if (_PWM_NULL_CHECK(h)) { _ret = ESP_ERR_INVALID_ARG; }\
    else { _ret = (h)->start(h); }                         \
    _ret;                                                   \
})
/**
 * @brief Stop the interleaved PWM output on all channels.
 *
 * Halts switching on all GPIO lines and drives them to their idle
 * level. The instance remains valid and can be restarted with
 * PWM_START without calling interleavedPWMCreate again.
 *
 * @param pwm  Pointer to an initialised interleaved_pwm_interface_t instance.
 
 * @return     0 on success, ESP_ERR_INVALID_ARG if pwm is NULL,
 *             non-zero on peripheral failure.
 *
 * @example
 *   PWM_STOP(pwm);
 */
#define PWM_STOP(h) ({                                      \
    int _ret;                                               \
    if (_PWM_NULL_CHECK(h)) { _ret = ESP_ERR_INVALID_ARG; }\
    else { _ret = (h)->stop(h); }                          \
    _ret;                                                   \
})

/**
 * @brief Destroy the interleaved PWM instance and release all resources.
 *
 * Stops the peripheral, resets all GPIO lines to their default input
 * state, and frees any internal allocations. The instance must not be
 * used after this call without calling interleavedPWMCreate again.
 *
 * Calling PWM_STOP before PWM_DESTROY is not required — destroy
 * handles a running instance safely.
 *
 * @param pwm  Pointer to an initialised interleaved_pwm_interface_t instance.
 
 *
 * @note  Returns void. Use PWM_STOP first if you need to check the
 *        return value before releasing resources.
 *
 * @example
 *   PWM_DESTROY(pwm);
 */
#define PWM_DESTROY(h) ({                                   \
    int _ret;                                               \
    if (_PWM_NULL_CHECK(h)) { _ret = ESP_ERR_INVALID_ARG; }\
    else { _ret = (*h)->destroy(h); }                       \
    _ret;                                                   \
})
/**
 * @brief Change the pulse width of a single channel at runtime.
 *
 * Updates the on-time of the specified channel. Can be called while
 * the PWM is running or stopped. The change takes effect immediately.
 *
 * The new width must satisfy the slot constraint:
 *
 *   pulse_width + dead_time <= time_period / total_gpio
 *
 * If the constraint is violated the call returns an error and the
 * channel continues operating at its previous pulse width unchanged.
 *
 * @param pwm  Pointer to an initialised interleaved_pwm_interface_t instance.
 
 * @param ch   Zero-based channel index (0 to total_gpio - 1).
 * @param w    New pulse width in microseconds.
 * @return     0 on success.
 *             ESP_ERR_INVALID_ARG if pwm is NULL or ch is out of range.
 *             Non-zero if w violates the slot constraint.
 *
 * @example
 *   // 2-channel, period=10000us, dead_time=1000us
 *   // slot = 10000/2 = 5000us  →  max width = 5000 - 1000 = 4000us
 *
 *   PWM_SET_WIDTH(pwm, 0, 2000);  // channel 0 → 20% duty  ✓
 *   PWM_SET_WIDTH(pwm, 1, 3500);  // channel 1 → 35% duty  ✓
 *   PWM_SET_WIDTH(pwm, 0, 9999);  // rejected  → unchanged  ✗
 */
#define PWM_SET_WIDTH(h, ch, w) ({                              \
    int _ret;                                                   \
    if (_PWM_NULL_CHECK(h)) { _ret = ESP_ERR_INVALID_ARG; }    \
    else { _ret = (h)->changePulseWidth((h), (ch), (w)); }     \
    _ret;                                                       \
})
/*The parameters are not in same units. For example time period is in microseconds where as dutycycle
is given in percentage instead of microseconds. So values in accepted in format which are convinient
to the enduser. And it is the job of the class to calculate/convert the values.
*/
typedef struct{

    uint8_t* gpio_no;
    uint32_t* pulse_widths;          //in microseconds. Array size equal to total_gpio. Width of each pulse. cannot be caluculated as complex requireent of distinct.
    uint8_t total_gpio;
    uint32_t time_period;           //in microseconds
    uint32_t dead_time;             //microseconds. Dead Time between each pulse. The phase is determined by this
}interleaved_pwm_config_t;



typedef struct interleaved_pwm_interface{
    int (*start)(struct interleaved_pwm_interface* self);
    int (*stop)(struct interleaved_pwm_interface* self);
    int (*destroy)(struct interleaved_pwm_interface** self);
    int (*changePulseWidth)(struct interleaved_pwm_interface* self,uint8_t channel_no, uint32_t pulse_widths);
    uint32_t (*getTimePeriod)();
}interleaved_pwm_interface_t;


//It needs to be changed. This  whole struct must be private and only interface must be returned



esp_err_t interleavedPWMCreate(interleaved_pwm_config_t* config,interleaved_pwm_interface_t** out_if);

#endif