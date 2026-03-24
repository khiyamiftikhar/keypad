/*There is no check to have exclusive pwm channel assignemnt for multiple instances
If multiple instances of pwm manager are to be created, they will still use use  from 
channel number 0 to total channel required. Actually a single channel can be associated with
multiple gpio in esp32, so all those gpio will have same pwm signal.
*/

#include "esp_log.h"
#include "esp_err.h"
#include "pwm_line.h"
#include <math.h>
#include "interleaved_pwm.h"


//static const uint8_t duty_one=100;
//static const uint8_t duty_two=100;



static const char* TAG="interleaved pwm";




#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


//A makeshift  arrangment to create only one instance

typedef struct {
    uint32_t time_period;
    void* lines;                //internally typcasted to  pwm_line_t. encapsulation
    uint8_t total_lines;
    uint32_t dead_time;             //microseconds. Dead Time between each pulse. The phase is determined by this
    interleaved_pwm_interface_t interface;
}interleaved_pwm_t;   

static struct{

    uint32_t time_period;




}interleaved_pwm_class_data;


static bool instance_created=false;

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

/// @brief  Allm channles get slot equally divided from total time_period
////////The pulse width + the required dead time before next pulse starts must not exceed the slot
/// @param pulse_widths 
/// @param dead_time 
/// @param slot_width 
/// @return 
static bool pulseWidthInvalid(uint32_t pulse_widths,
                           uint32_t dead_time,
                           uint32_t slot_width){

    if((pulse_widths + dead_time) > slot_width)
        return true;

    return false;

}

static int pulseWidthCheck(uint32_t* pulse_widths,
                           uint8_t total_gpio,
                           uint32_t dead_time,
                           uint32_t time_period)
{
    if(total_gpio == 0)
        return ERR_PROBE_MANAGER_WRONG_PARAMETERS;

    uint32_t slot = time_period / total_gpio;

    for(uint8_t i = 0; i < total_gpio; i++)
    {
        //If width + dead time exceeds the slot 
        if(pulseWidthInvalid(pulse_widths[i],dead_time,slot))
            return ERR_PROBE_MANAGER_WRONG_PARAMETERS;
    }

    return 0;
}




static int start(interleaved_pwm_interface_t* self){

    if(self==NULL)    
        return ESP_FAIL;

    interleaved_pwm_t* prb=container_of(self,interleaved_pwm_t,interface);

    pwm_line_t* lines=(pwm_line_t*) prb->lines;
    if(lines==NULL)
        return ESP_FAIL;
    uint8_t total_lines=prb->total_lines;

    for(int8_t i=(total_lines-1);i>=0;i--){
        lines[i].interface.pwmStart(&lines[i].interface);
    }

    return 0;
}


static int stop(interleaved_pwm_interface_t* self){

    if(self==NULL)    
        return ESP_FAIL;

    interleaved_pwm_t* prb=container_of(self,interleaved_pwm_t,interface);

    pwm_line_t* lines=(pwm_line_t*) prb->lines;
    if(lines==NULL)
        return ESP_FAIL;
    uint8_t total_lines=prb->total_lines;

    for(uint8_t i=0;i<total_lines;i++){
        lines[i].interface.pwmStop(&lines[i].interface);
    }
   

    return 0;
}


static int changeWidth(interleaved_pwm_interface_t* self,uint8_t channel_no,uint32_t pulse_width){

    
    if(self==NULL)    
        return ESP_FAIL;
    interleaved_pwm_t* prb=container_of(self,interleaved_pwm_t,interface);

    pwm_line_t* lines=(pwm_line_t*) prb->lines;

    if(lines==NULL)
        return ESP_FAIL;
    uint8_t total_lines=prb->total_lines;

    //Check channel_no>(total_lines-1) because channel number s start from 0
    if(channel_no<0 || channel_no>(total_lines-1))
        return ESP_FAIL;

    ESP_LOGI(TAG,"changing total_lines %d  slot width %d, channel_no %d",total_lines,prb->time_period/prb->total_lines,channel_no);
    if(pulseWidthInvalid(pulse_width,prb->dead_time,prb->time_period/prb->total_lines))
        return ESP_FAIL;

    lines[channel_no].interface.pwmChangeWidth(&lines[channel_no].interface,pulse_width,prb->time_period);
    //for(uint8_t i=0;i<total_lines;i++){
      //  lines[i].interface.pwmStop(&lines[i].interface);
    //}

    return 0;
}


static int destroy(interleaved_pwm_interface_t* self)
{
    if (self == NULL)
        return ESP_FAIL;

    interleaved_pwm_t* prb = container_of(self, interleaved_pwm_t, interface);

    ESP_LOGI(TAG,"interface address %p",self);

    if (prb == NULL)
        return ESP_FAIL;

    pwm_line_t* lines = prb->lines;

    if (lines == NULL)
        return ESP_FAIL;

    ESP_LOGI(TAG, "destroying prober, total lines %d", prb->total_lines);

    for (uint8_t i = 0; i < prb->total_lines; i++)
    {
        lines[i].interface.pwmDestroy(&lines[i].interface);
    }

    
    free(lines);
    prb->lines = NULL;

    /* free the main object */
    free(prb);

    instance_created = false;

    return 0;
}

/// @brief Free the user pointer using double pointer, and then call the normal destroy method
///         Required after the malloc becaue user pointer is  not cleared by simple destroy
/// @param pwm 
/// @return 
int destroyMaster(interleaved_pwm_interface_t** pwm)
{
    if (!pwm || !*pwm)
        return ESP_ERR_INVALID_ARG;

    interleaved_pwm_interface_t *obj = *pwm;

    int ret = destroy(obj);

    *pwm = NULL;   //  critical: prevent dangling pointer

    return ret;
}
/**
 * @brief Compute optimal LEDC timer resolution for interleaved PWM
 *
 * This function selects the highest possible PWM resolution that satisfies:
 *
 * 1. Hardware constraint:
 *    The requested PWM frequency must be achievable given the LEDC clock.
 *
 *        f_pwm ≤ f_clk / (2^resolution)
 *
 * 2. Interleaving quality constraint:
 *    Each interleaved slot must have sufficient timing granularity so that
 *    pulse widths can be meaningfully controlled.
 *
 *        time_step ≤ slot_width × threshold
 *
 *    Which simplifies to:
 *
 *        2^resolution ≥ channels / threshold
 *
 * The function computes:
 *
 *    resolution_max = floor(log2(f_clk / f_pwm))
 *    resolution_min = ceil(log2(channels / threshold))
 *
 * and selects the maximum valid resolution in the range:
 *
 *    resolution_min ≤ resolution ≤ resolution_max
 *
 * If no valid resolution exists, the configuration is rejected.
 *
 * @param freq_hz        Desired PWM frequency in Hz
 * @param channels       Number of interleaved PWM channels
 * @param threshold      Minimum ratio of slot width to timer step,
 *                       (e.g., 0.1 = at least 10 steps per slot)
 *                       if timer step is bigger than this than wave generation not accurate
 *                       For example if slot width i 1ms and timer step is > 0.1 m 
 *                       then it is not accurate enough
 *
 * @return
 *    >= 0 : Selected resolution in bits
 *    -1   : No valid resolution exists for given parameters
 *
 * @note
 * - Assumes LEDC clock is 80 MHz
 * - Higher resolution provides finer duty control
 * - Lower resolution allows higher PWM frequencies
 */

static int compute_resolution(uint32_t freq, uint8_t channels, float threshold)
{
    const uint32_t clk = 80000000;

    int res_max = floor(log2((double)clk / freq));
    int res_min = ceil(log2((double)channels / threshold));

    ESP_LOGI(TAG,"res max %d re_min %d",res_max,res_min);

    if (res_min > res_max)
        return -1;

    return res_max;
}

uint32_t getTimePeriod(){

    return interleaved_pwm_class_data.time_period;
}

esp_err_t interleavedPWMCreate(
    interleaved_pwm_config_t* config,
    interleaved_pwm_interface_t** out_if)
{
    if (config == NULL || out_if == NULL ||
        config->gpio_no == NULL ||
        config->pulse_widths == NULL ||
        config->time_period == 0 ||
        config->total_gpio == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "creating instance");

    if (instance_created == true) {
        ESP_LOGW(TAG, "Already running, only one instance supported");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t total_gpio       = config->total_gpio;
    uint32_t time_period     = config->time_period;
    uint32_t dead_time       = config->dead_time;
    uint32_t* pulse_widths   = config->pulse_widths;
    uint8_t* gpio_no         = config->gpio_no;

    int frequency = 1000000 / time_period;

    if (total_gpio > pwmGetMaxChannels()) {
        ESP_LOGE(TAG, "Exceeds max channels supported");
        return ESP_ERR_NOT_SUPPORTED;
    }

    int timer_resolution = compute_resolution(frequency, total_gpio, 0.05);
    if (timer_resolution <= 0) {
        ESP_LOGE(TAG, "Frequency too high for given slot width");
        return ESP_ERR_INVALID_ARG;
    }

    if (timer_resolution > pwmGetMaxTimerReolution()) {
        timer_resolution = pwmGetMaxTimerReolution();
    }

    int ret = pulseWidthCheck(pulse_widths, total_gpio, dead_time, time_period);
    if (ret != 0) {
        ESP_LOGE(TAG, "Pulse width + deadtime exceed slot width");
        return ESP_ERR_INVALID_ARG;
    }

    int phase = phaseCalculate(total_gpio);

    interleaved_pwm_t* interleaved_pwm = malloc(sizeof(interleaved_pwm_t));
    if (interleaved_pwm == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed (interleaved_pwm)");
        return ESP_ERR_NO_MEM;
    }

    pwm_line_t* pwm_line = malloc(sizeof(pwm_line_t) * total_gpio);
    if (pwm_line == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed (pwm_line)");
        free(interleaved_pwm);
        return ESP_ERR_NO_MEM;
    }

    interleaved_pwm->lines = pwm_line;
    interleaved_pwm->total_lines = total_gpio;

    pwm_config_t line_config;
    uint32_t current_phase = 0;

    for (uint8_t i = 0; i < total_gpio; i++)
    {
        line_config.pulse_width     = pulse_widths[i];
        line_config.channel_number  = i;
        line_config.dead_time       = 0;
        line_config.gpio            = gpio_no[i];
        line_config.phase           = current_phase;
        line_config.time_period     = time_period;
        line_config.timer_resolution= timer_resolution;

        esp_err_t err = pwmCreate(&pwm_line[i], &line_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "pwmCreate failed at channel %d", i);

            // cleanup already created channels
            for (uint8_t j = 0; j < i; j++) {
                pwm_line[j].interface.pwmDestroy(&pwm_line[j].interface);
            }

            free(pwm_line);
            free(interleaved_pwm);
            return err;
        }

        current_phase += phase;
        ESP_LOGI(TAG, "creating channel %d phase %lu", i, current_phase);
    }

    interleaved_pwm->interface.start            = start;
    interleaved_pwm->interface.stop             = stop;
    interleaved_pwm->interface.destroy          = destroyMaster;
    interleaved_pwm->interface.changePulseWidth = changeWidth;
    interleaved_pwm->interface.getTimePeriod=  getTimePeriod;

    interleaved_pwm->time_period = time_period;
    interleaved_pwm->dead_time   = dead_time;

    instance_created = true;

    interleaved_pwm_class_data.time_period=time_period;

    *out_if = &interleaved_pwm->interface;

    ESP_LOGI(TAG, "interleaved PWM created");

    return ESP_OK;
}


