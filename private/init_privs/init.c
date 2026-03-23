#include "esp_log.h"
#include "init.h"

#define MAX_KEYPADS              CONFIG_MATRIX_KEYPAD_MAX_KEYPADS
#define MAX_SIMULTANEOUS_KEYS    CONFIG_MATRIX_KEYPAD_MAX_SIMULTANEOUS_KEYS
#define MAX_BUTTONS             (CONFIG_MATRIX_KEYPAD_MAX_COLS*CONFIG_MATRIX_KEYPAD_MAX_ROWS)


static const char* TAG="init";

esp_err_t configKeypadButtons(button_interface_t** buttons,
                                uint8_t* keymap,
                                uint8_t total_buttons,
                                pool_alloc_interface_t* timer_pool,
                                uint32_t prober_time_period,
                                buttonCallBack buttonEventHandler,
                                void* context){

    button_config_t config={0};
    config.scan_time_period=prober_time_period + 6000; //If no pulse comes for this duraition for a pressed button, then it will be assumned to be relased
    ESP_LOGI(TAG,"timer period %"PRIu32,prober_time_period);
    config.cb=buttonEventHandler;
    config.context=(void*)context;
    config.timer_pool=timer_pool;
    
    
    
    for(uint8_t i=0;i<total_buttons;i++){
        
        config.button_index=i;  
        config.button_id=keymap[i];
        buttons[i]=keypadButtonCreate(&config);
        if(buttons[i]==NULL)
            return ESP_FAIL;
        
    }


    return ESP_OK;

}



/*
 * Computes evenly spaced pulse widths for a matrix keypad row
 * and the minimum valid PWM period for those widths.
 *
 * BASE_WIDTH_US is the most important parameter — it acts as a
 * hardware-level bounce filter. Individual bounce pulses from
 * mechanical/membrane switches are typically short (< 1 ms).
 * Setting BASE_WIDTH_US above the worst-case individual bounce
 * pulse width ensures the decoder never mistakes a bounce pulse
 * for a legitimate signal.
 *
 * Recommended values by switch type:
 *   Membrane keypad : BASE_WIDTH_US >= 2000 µs  (bounce pulses < 1 ms)
 *   Tactile PCB     : BASE_WIDTH_US >= 2000 µs  (bounce pulses < 1 ms)
 *   Cheap dome      : BASE_WIDTH_US >= 3000 µs  (bounce pulses < 2 ms)
 *
 * Note: BASE_WIDTH_US filters short bounce pulses only. A separate
 * software debounce in the callback is still recommended to handle
 * repeated triggers during the full bounce window (~20 ms).
 *
 * Rule:
 *   BASE_WIDTH_US > max_individual_bounce_pulse + tolerance_us
 *   STEP_US       > 2 * tolerance_us   (signals must not overlap)
 *
 * @param total_signals  Number of columns (e.g. 4 for a 4x4 keypad)
 * @param dead_time_us   Dead time that will be used in the PWM config
 * @param out_widths     Empty array of size total_signals — filled by this function
 * @param out_period_us  Minimum valid period — pass directly to PWM config
 */

#define KEYPAD_BASE_WIDTH_US   2000u   /* µs — must exceed max bounce pulse width  */
#define KEYPAD_STEP_US          400u   /* µs — spacing between adjacent signals    */
#define KEYPAD_PERIOD_ROUND_US 1000u   /* µs — round period to nearest clean value */

static void keypadComputePulseTimings(uint8_t   total_signals,
                          uint32_t  dead_time_us,
                          uint32_t* out_widths,
                          uint32_t* out_period_us)
{
    for (uint8_t i = 0; i < total_signals; i++)
    {
        out_widths[i] = KEYPAD_BASE_WIDTH_US + (i * KEYPAD_STEP_US);
    }

    /*
     * slot must satisfy: max_width + dead_time <= slot
     * period = total_signals * slot
     * rounded up to nearest KEYPAD_PERIOD_ROUND_US
     */
    uint32_t max_width  = out_widths[total_signals - 1];
    uint32_t min_slot   = max_width + dead_time_us;
    uint32_t min_period = (uint32_t)total_signals * min_slot;

    *out_period_us = ((min_period + KEYPAD_PERIOD_ROUND_US - 1)
                      / KEYPAD_PERIOD_ROUND_US) * KEYPAD_PERIOD_ROUND_US;
}





//This same array is used both by prober and scanner



esp_err_t configKeypadOutput(interleaved_pwm_interface_t** self,uint8_t* output_gpio,uint32_t* pulse_widths,uint8_t total_outputs){

    interleaved_pwm_config_t config={0};

    config.gpio_no=output_gpio;
    config.total_gpio=total_outputs;
    
    uint32_t pwm_widths[total_outputs];
    uint32_t time_period;
    uint32_t dead_time=500;
    keypadComputePulseTimings(total_outputs,dead_time,pwm_widths,&time_period);
    
    config.pulse_widths=pulse_widths;
    config.time_period=time_period;
    config.dead_time=dead_time;          //1000 microseconds

    config.time_period=time_period;   //(2000+1000)+(2400+1000)+(2800+1000)+(3200+1000
    
    //initalizing the prober member of self. It needs to be changed to have internal static allocation inside scanner
    int ret=interleavedPWMCreate(&config,self);

    return ret;

}

esp_err_t configKeypadTimers(timer_interface_t **timers,  uint8_t total_timers,  void *context,buttonCallBack timerEventHandler){
    char name[10];

    for (uint8_t i = 0; i < total_timers; i++) {

        sprintf(name, "timer %d", i);

        timers[i] = timerCreate(name, timerEventHandler, context);

        if (timers[i] == NULL)
            return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t configTimerPool(pool_alloc_interface_t** out ,timer_interface_t** timers,uint8_t total_objects){

    //This call does not require any paramaters bcz Max objects that a pool can manage is the only requirement
    //which is set through Kconfig
    pool_alloc_interface_t* pool=poolQueueCreate();

    
    if(pool==NULL)
        return ESP_FAIL;

    //Now fill the pool
    for(uint8_t i=0;i<total_objects;i++){

        if(timers[i]==NULL)
            return ESP_FAIL;
        else
        
            pool->poolFill(pool,(void*)timers[i]);

    }
    
    *out=pool;

    return ESP_OK;

}

/// @brief Configure the scanner that reads the inputs of matrix keypad
/// The self pointer is double, so that context contains the address of scanner member and thus can extract keypad_dev_t self from context
/// @param input_gpio   The list of gpio associated
/// @param total_inputs Total gpios
/// @param total_outputs Total outputs of matrix keypad, each with different pwm signal and thus different signal
/// @return 


esp_err_t configKeypadInput(scanner_interface_t** self,
                            uint8_t* input_gpio,
                            uint8_t total_inputs,   //Total loop interations
                            uint8_t total_outputs,  //Each line will detect this many pulses
                            uint32_t* pulse_widths_us,
                            scannerCallBack scannerEventHandler,
                            void* context) //The keypad object, because multiple keppad can be crrated
{

    scanner_config_t config={0};
    
    

    
    
        config.gpio_no=input_gpio;
        config.total_signals=total_outputs;
        config.tolerance=100;       //microseconds so  0.1 milliseconds
        config.pwm_widths_array=pulse_widths_us;
        config.context=(void*)context; //This will later reterive the keypad_dev_t instance
        config.cb=scannerEventHandler;
        
        
        //Assigning the scanner member of the self
        esp_err_t ret=scannerCreate(&config,self);

        if(ret!=NULL)
            return ESP_FAIL;

        return ESP_OK;

    
}


