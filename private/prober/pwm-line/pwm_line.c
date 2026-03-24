
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "pwm_line.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_sig_map.h"



static const char* TAG="pwm line";





#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})




#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          (100) // Frequency in Hertz. Set frequency at 100Hz so 10 ms

#define PWM_MAX_CHANNELS        (LEDC_CHANNEL_MAX+1)


static bool timer_init_done=false;   //so that get initialized only once

static int percentage_to_ticks(int percentage,int total_ticks){

    return (percentage*total_ticks)/100;

}



/// @brief Convert %age duty to timer counts. Also reduces if there is a duty cut to have gap between phases 
/// @param duty     in %age
/// @param timer_size timer bits
/// @param duty_cut in %age
/// @return 
static uint32_t pulseWidthToTicks(uint32_t pulse_width,uint32_t time_period,uint8_t timer_size){

    int total_ticks=(1<<timer_size); //2^13

    uint32_t duty_ticks = pulse_width*total_ticks/time_period;
    //int duty_ticks=percentage_to_ticks(duty,total_ticks);

    //int duty_cut_ticks=percentage_to_ticks(duty_cut,duty_ticks);

    return (duty_ticks);

}


/// @brief Convert %age duty to timer counts. Also reduces if there is a duty cut to have gap between phases 
/// @param phase     in degrees
/// @param timer_size timer bits
/// @return 
static int pwmLagTicksCalculate(uint16_t phase,uint32_t dead_time,uint32_t time_period, uint8_t timer_size){

    int total_ticks=(1<<timer_size); //2^13
    int phase_percentage=((int)phase*100)/360;               //Phase_in_Degrees/Total_Degress * 100
    int dead_time_percentage=dead_time*100/time_period;

    int phase_ticks=((phase_percentage+dead_time_percentage)*total_ticks)/100;
    ESP_LOGI(TAG,"phase percentage %d, ticks %d",phase_percentage,phase_ticks);

    return phase_ticks;

}

/* Detach — GPIO goes idle, LEDC channel keeps running internally */
static void pwm_detach_gpio(uint8_t gpio_num)
{
    gpio_reset_pin(gpio_num);
}

/* Attach — connects GPIO to the LEDC channel output signal */
static void pwm_attach_gpio(uint8_t gpio_num, ledc_channel_t channel)
{
    /* LEDC_LS_SIG_OUT0_IDX + channel gives the signal index for each channel */
    uint32_t signal_idx = LEDC_LS_SIG_OUT0_IDX + channel;

    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal(gpio_num, signal_idx, false, false);
}


static void pwmStop(pwm_line_interface_t* self){

    pwm_line_t* pwm_line=container_of(self,pwm_line_t,interface);
    ledc_stop(LEDC_MODE,pwm_line->channel_number,1);
    pwm_detach_gpio(pwm_line->gpio_number);

}


static void pwmStart(pwm_line_interface_t* self){

    pwm_line_t* pwm_line=container_of(self,pwm_line_t,interface);

    ledc_set_duty_with_hpoint(LEDC_MODE,pwm_line->channel_number,pwm_line->duty,pwm_line->hpoint);
    ledc_update_duty(LEDC_MODE, pwm_line->channel_number);
    uint32_t delay_ms = (pwm_line->time_period * 6) / 1000;
    //ESP_LOGI(TAG,"delay %lu",delay_ms);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    pwm_attach_gpio(pwm_line->gpio_number, pwm_line->channel_number);

}


static void pwmChangeWidth(struct pwm_line_interface* self,uint32_t pulse_width_us,uint32_t time_period){   //in microseconds



    pwm_line_t* pwm_line=container_of(self,pwm_line_t,interface);
    //Convert to Ticks
    uint32_t duty_ticks= pulseWidthToTicks(pulse_width_us,time_period,LEDC_DUTY_RES);
    //Set the duty member
    pwm_line->duty=duty_ticks;
    //Start with new duty
    pwmStart(self);
}

static void pwmDestroy(pwm_line_interface_t* self)
{
    pwm_line_t* pwm_line = container_of(self, pwm_line_t, interface);

    ledc_stop(LEDC_MODE, pwm_line->channel_number, 0);

    gpio_reset_pin(pwm_line->gpio_number);
    timer_init_done=true;// not good, will clear on only one line deletion

}



/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2 (rev < 1.2), ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */

int pwmCreate(pwm_line_t* self,pwm_config_t*  config){
    // Prepare and then apply the LEDC PWM timer configuration
    
    uint32_t time_period=config->time_period;
    uint32_t dead_time = config->dead_time;



    uint32_t frequency=1000000/time_period;     //1M/(time period in microseconds)

    if(timer_init_done==false){
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_MODE,
            .duty_resolution  = config->timer_resolution,
            .timer_num        = LEDC_TIMER,
            .freq_hz          = frequency,
            .clk_cfg          = LEDC_AUTO_CLK
        };
        if(ledc_timer_config(&ledc_timer)!=0)
            return -1;

        timer_init_done=true;
    }

    uint32_t duty_ticks=pulseWidthToTicks(config->pulse_width,time_period,LEDC_DUTY_RES);
    int lag=pwmLagTicksCalculate(config->phase,dead_time,time_period,LEDC_DUTY_RES);
    // Prepare and then apply the LEDC PWM channel configuration
    //There are two modes high and low, and 8 channel for each
    //So channel member can have value 0-7 but speed_mode tells channel no of which device, low or high
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = config->channel_number,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = config->gpio,
        .duty           = duty_ticks,
        .hpoint         = lag
    };

    //ESP_LOGI(TAG,"before channel config");
    if(ledc_channel_config(&ledc_channel)!=0)
        return -1;
    //ESP_LOGI(TAG,"after channel config");

    self->channel_number=config->channel_number;
    self->gpio_number=config->gpio;
    self->hpoint=lag;
    self->duty=duty_ticks;
    self->time_period=time_period;
    self->interface.pwmStart=pwmStart;
    self->interface.pwmStop=pwmStop;
    self->interface.pwmDestroy=pwmDestroy;
    self->interface.pwmChangeWidth=pwmChangeWidth;

    //ESP_LOGI(TAG,"returning from pwm_line");

    return 0;
    
}



int pwmGetMaxChannels(){
    return PWM_MAX_CHANNELS;
}

int pwmGetMaxTimerReolution(){
 return LEDC_DUTY_RES;   
}