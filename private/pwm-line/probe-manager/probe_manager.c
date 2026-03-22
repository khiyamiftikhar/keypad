/*There is no check to have exclusive pwm channel assignemnt for multiple instances
If multiple instances of pwm manager are to be created, they will still use use  from 
channel number 0 to total channel required. Actually a single channel can be associated with
multiple gpio in esp32, so all those gpio will have same pwm signal.
*/

#include "esp_log.h"
#include "esp_err.h"
#include "pwm_line.h"
#include "probe_manager.h"


//static const uint8_t duty_one=100;
//static const uint8_t duty_two=100;



static const char* TAG="pwm line";




#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})




static int start(prober_interface_t* self){

    prober_t* prb=container_of(self,prober_t,interface);

    pwm_line_t* lines=(pwm_line_t*) prb->lines;
    uint8_t total_lines=prb->total_lines;

    for(uint8_t i=0;i<total_lines;i++){
        lines[i].interface.pwmStart(&lines->interface);
    }

    return 0;
}


static int stop(prober_interface_t* self){

    prober_t* prb=container_of(self,prober_t,interface);

    pwm_line_t* lines=(pwm_line_t*) prb->lines;
    uint8_t total_lines=prb->total_lines;

    for(uint8_t i=0;i<total_lines;i++){
        lines[i].interface.pwmStop(&lines->interface);
    }

    return 0;
}

//not used
static int proberCheckDeadTime(uint32_t time_period,uint32_t dead_time){
    uint32_t percentage=(dead_time*100)/time_period;

    //Greater than 5%
    if(percentage>5)    
        return ERR_PROBE_MANAGER_DEAD_TIME_LARGE;
    //if(percentage<)
    return 0;
}


static int frequencyCheck(uint32_t frequency){

    if(frequency>100)   //100Hz because it gives  10ms. less than 10ms is less time for 4 input keypad
        return ERR_PROBE_MANAGER_FREQUENCY_LARGE;

    return 0;
}

static int phaseCalculate(uint8_t total_gpio){
    if(total_gpio<=0)
        return ERR_PROBE_MANAGER_WRONG_PARAMETERS; 
    return 360/total_gpio;
}

static int pulseWidthCheck(uint32_t* pulse_widths,uint8_t total_gpio,uint32_t dead_time,uint32_t time_period){

    uint16_t total_time=0;

    for(uint8_t i=0;i<total_gpio;i++)
        total_time+=pulse_widths[i]+dead_time;

    if(total_time>time_period)
        return ERR_PROBE_MANAGER_WRONG_PARAMETERS;
    return 0;
}

int proberCreate(prober_t* self,prober_config_t* config){

    if(self==NULL || config==NULL)
        return ERR_PROBE_MANAGER_INVALID_MEM;

    uint8_t total_gpio=config->total_gpio;
    uint32_t time_period=config->time_period;        //Time period in microseconds
    int frequency=1000000/time_period;
    uint32_t* pulse_widths=config->pulse_widths;
    uint32_t dead_time=config->dead_time;
    uint8_t* gpio_no=config->gpio_no;

    int ret=pulseWidthCheck(pulse_widths,total_gpio,dead_time,time_period);

    if(ret!=0)
        return ret;

    ret=frequencyCheck(frequency);
    
    if(ret!=0)
        return ret;

    
    int phase=phaseCalculate(total_gpio);
    

    
    pwm_line_t * pwm_line = (pwm_line_t*) malloc(sizeof(pwm_line_t)*total_gpio);

    if(pwm_line==NULL)
        return ESP_ERR_NO_MEM;
    self->lines=pwm_line;


    pwm_config_t line_config;
    uint16_t current_phase=0;
    for(uint8_t i=0;i<total_gpio;i++){
        line_config.pulse_width=pulse_widths[i];
        line_config.channel_number=i;
        line_config.dead_time=dead_time;
        line_config.gpio=gpio_no[i];
        line_config.phase=current_phase;
        line_config.channel_number=i;
        line_config.time_period=time_period;
        ESP_ERROR_CHECK(pwmCreate(&pwm_line[i],&line_config));
        current_phase+=phase;
    }

    self->interface.start=start;
    self->interface.stop=stop;
    self->time_period=config->time_period;

    ESP_LOGI(TAG,"bye bye");       
    return 0;

}