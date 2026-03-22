/*


There is no explicit way to tell the channel required. The channel api of capture unit keeps track 
of the channels assigned internally and automatically assigns the next channel based on the timer group
provided. So if timer group zero is used in three calls, all three channels will be provided one by one.
When the 4th call will be made it will fail. So next timer group should be provided in call




*/

#include <string.h>
#include "math.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "pwm_capture.h"

static const char* TAG="capture";

#define         MAX_CHANNELS                6
#define         MAX_CHANNELS_PER_UNIT       3
#define         QUEUE_LENGTH                100

/*
size_t captureGetSize(){
    size_t size=sizeof(pwm_capture_t);
    //Now allocating memory for the pointers used in the struct
    size+=sizeof(uint32_t);
    size+=sizeof(class_data_t);
    Cannot proceed. Class_data_t contains pointers in its members. then mem for those needs to
    be allocated. Those members are structs and they further contain pointer members. so it seems
    never ending
}
*/



typedef struct capture_event_data{
    const pwm_capture_t* cap_obj;
    uint32_t pulse_width_ticks;       //in ticks

}capture_event_data_t;


#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



static int startMonitoring(struct capture_interface* self){


    pwm_capture_t* cap_obj=CONTAINER_OF(self,pwm_capture_t,interface);
    
    int ret=0;
    ESP_LOGI(TAG, "Enable capture channel");
    ret=mcpwm_capture_channel_enable(cap_obj->cap_chan);
    if(ret!=0)
        return ret;

    ret=mcpwm_capture_timer_enable(cap_obj->cap_timer);
    if(ret!=0)
        return ret;
    ret=mcpwm_capture_timer_start(cap_obj->cap_timer);

    return ret;
}

static int stopMonitoring(struct capture_interface* self){

    pwm_capture_t* cap_obj=CONTAINER_OF(self,pwm_capture_t,interface);
    return mcpwm_capture_channel_disable(cap_obj->cap_chan);
}






static uint32_t calculatePulseTime(uint32_t timeTicks){

    uint32_t pulse_width_us = (timeTicks * 1000000.0) / esp_clk_apb_freq();

    return pulse_width_us;
}

static int determinePulseId(uint32_t* standard_pwm_widths,uint8_t total_signals,uint32_t received_pulse_width,uint32_t tolerance){



    for(uint8_t i=0;i<total_signals;i++){

        if(abs(standard_pwm_widths[i]-received_pulse_width)<tolerance)
            return i;

    }

    return ERR_CAPTURE_UNREGISTERD_PULSE_WIDTH;

}


static void task_processCaptureQueue(void* args){



    pwm_capture_t * self=(pwm_capture_t *)args;
    pwm_capture_class_data_t* class_data = self->class_data;

    uint32_t min_width=class_data->min_width;
    uint32_t* standard_pwm_widths=class_data->pulse_widths;
    uint32_t tolerance=class_data->tolerance;
    uint8_t total_signals=class_data->total_signals;
    callbackForCapture cb=class_data->callback;
    //pwm_capture_t* cap_obj=evt_data->cap_obj;

    capture_event_data_t capture_evt_data;
    scanner_event_data_t scanner_evt_data;

    //QueueHandle_t queue=self->queue;

    while(1){

        if(xQueueReceive(self->queue,&capture_evt_data,portMAX_DELAY)==pdTRUE){

           // ESP_LOGI(TAG,"wow");
            //Calculate Time in us from ticks
            uint32_t received_pulse_width=calculatePulseTime(capture_evt_data.pulse_width_ticks);

            //ESP_LOGI(TAG,"PW %lu",received_pulse_width);

            if(received_pulse_width<min_width)
                continue;

            int id=determinePulseId(standard_pwm_widths,total_signals,received_pulse_width,tolerance);

            //If true it means a match is found
            if(id>=0){

                scanner_evt_data.line_number=capture_evt_data.cap_obj->index;
                scanner_evt_data.source_number=id;
                cb(&scanner_evt_data,class_data->context);  //So this is carrying the weight till user is informed

            }
            

            

        }


    }
}


/*

size_t captureGetClassDataSize(uint8_t total_channels,uint8_t total_signals){

    size_t size;

    
    size+=sizeof(pwm_capture_class_data_t);

    //The logic is rigid. It assumes there are 3 channels per group, and 2 groups available so esp32
      //  The esp32c3 or h3 are not considered
    
    if(total_channels>MAX_CHANNELS)
        return -1;
    else if(total_channels>MAX_CHANNELS_PER_UNIT)
        size+=sizeof(cap_timer_t)*2;
    else
        size+=sizeof(cap_timer_t);


    return size;
}

*/

//int captureMemAssign(pwm_capture_t total_channels,uint8_t total_signals){



/// @brief This is the callback registered with the ESPIDF driver
/// @param cap_chan 
/// @param edata 
/// @param user_data 
static bool captureCallback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    
    pwm_capture_t* self=(pwm_capture_t*)user_data;    
    uint32_t* cap_val_begin_of_sample = &self->time_stamp;
    uint32_t cap_val_end_of_sample = 0;
    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;


//    ESP_LOGI(TAG,"teri");
    //calculate the interval in the ISR,
    //so that the interval will be always correct even when capture_queue is not handled in time and overflow.
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        // store the timestamp when pos edge is detected
        *cap_val_begin_of_sample = edata->cap_value;
        //cap_val_end_of_sample = cap_val_begin_of_sample;
    } else {
        cap_val_end_of_sample = edata->cap_value;
        uint32_t tof_ticks = cap_val_end_of_sample - *cap_val_begin_of_sample;

        capture_event_data_t evt_data={.cap_obj=self,
                                        .pulse_width_ticks=tof_ticks
                                        };
        // notify the task to calculate the distance
        xQueueSendFromISR(self->queue,&evt_data,&high_task_wakeup);
        portYIELD_FROM_ISR(high_task_wakeup); // Switch to the woken task ASAP
    }


    return high_task_wakeup == pdTRUE;
}


static int timerCreate(mcpwm_cap_timer_handle_t* cap_timer,mcpwm_capture_timer_config_t* cap_timer_config, uint8_t group_id){


    cap_timer_config->clk_src= MCPWM_CAPTURE_CLK_SRC_DEFAULT;
    cap_timer_config->group_id = group_id;
    

    ESP_ERROR_CHECK(mcpwm_new_capture_timer(cap_timer_config, cap_timer));

    return 0;

}






int captureClassDataInit(pwm_capture_class_data_t* self,uint32_t min_width, uint32_t tolerance,uint32_t* pwm_widths_array,uint8_t total_gpio,uint8_t total_signals, callbackForCapture cb, void* context){

    int ret=0;
    if(self==NULL || cb==NULL || context==NULL)
        return ERR_CAPTURE_INVALID_ARGS;

     //Very rigig logid. Fit For esp32, which has 2 MCPWM groups each supporting 3 capture units
    if(total_gpio>MAX_CHANNELS)
        return -1;
    else if(total_gpio>MAX_CHANNELS_PER_UNIT){
        self->timer=(cap_timer_t*)malloc(sizeof(cap_timer_t)*2);

        memset(self->timer,0,sizeof(cap_timer_t)*2);
    }
    else{
        self->timer=(cap_timer_t*)malloc(sizeof(cap_timer_t));
        memset(self->timer,0,sizeof(cap_timer_t));
    }

    mcpwm_cap_timer_handle_t* cap_timer = &self->timer->cap_timer;
    mcpwm_capture_timer_config_t* cap_timer_config=&self->timer->cap_conf;


    
    timerCreate(cap_timer,cap_timer_config,0);  //Create timer of mcpwm of group 0

    
    //If there are more than 3 channels required then also create the other timer i.e of group 1
    if(total_gpio>MAX_CHANNELS_PER_UNIT){

        cap_timer = &self->timer[1].cap_timer;
        cap_timer_config=&self->timer[1].cap_conf;

        timerCreate(cap_timer,cap_timer_config,1);  //Create timer of mcpwm of group 1

    }

    
    self->pulse_widths=(uint32_t*) malloc(sizeof(uint32_t)*total_signals);

    if(self->pulse_widths==NULL)
        return ERR_CAPTURE_MEM_ALLOC;

    self->total_signals=total_signals;
    self->count=0;
    self->min_width=min_width;
    memcpy(self->pulse_widths,pwm_widths_array,sizeof(uint32_t)*total_signals);

    self->callback=cb;
    self->tolerance=tolerance;
    self->context=context;
    

    
    

    return 0;
}





/// @brief This is to be called whenever an instance of capture object is creatted. It is called after the
///         class data is already created which is common for all instances
/// @param self 
/// @param gpio 
/// @return 
int captureCreate(pwm_capture_t* self,  pwm_capture_class_data_t* class_data,uint8_t gpio){

    if(self==NULL || class_data==NULL)
        return -1;

    self->class_data=class_data;    
    //If true it means initialization not done
    if(self->class_data->timer==NULL)
        return ERR_CAPTURE_INVALID_ARGS;
    


    self->gpio_num=gpio;

    uint8_t group_id;

    mcpwm_cap_timer_handle_t* cap_timer;
    if(self->class_data->count==MAX_CHANNELS)
        return ERR_CAPTURE_CAP_UNIT_EXCEED;
    /*Because each group has 3 timers and 3 channels. There is no id selection in channel config
    and distinct channel is requried for each gpio, so total allocated count is checked to move 
    to a new group  if all 3 are allocated*/
    
    else if(self->class_data->count<MAX_CHANNELS_PER_UNIT){
        group_id=0;     //not used
        cap_timer = &self->class_data->timer->cap_timer;
        //Assign the instance cap_timer to the appropriater timer handle
        self->cap_timer=*cap_timer;
    }
    else{
        ESP_LOGI(TAG,"group 2");
        group_id=1;     
        cap_timer = &self->class_data->timer[1].cap_timer;
        //Assign the instance cap_timer to the appropriater timer handle
        self->cap_timer=*cap_timer;
    }
    

    ESP_LOGI(TAG, "Assign Timer timer");
    

    
    
    ESP_LOGI(TAG, "Install capture channel");
    //Pointer to handle, so double pointer
    mcpwm_cap_channel_handle_t* cap_chan = &self->cap_chan;
    mcpwm_capture_channel_config_t* cap_ch_conf=&self->cap_ch_conf;
    cap_ch_conf->gpio_num = gpio;
    cap_ch_conf->prescale = 1;
    // capture on both edge
    cap_ch_conf->flags.neg_edge = true;
    cap_ch_conf->flags.pos_edge = true;
    // pull up internally
    cap_ch_conf->flags.pull_up = false;
    
    
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(*cap_timer, cap_ch_conf, cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    
    mcpwm_capture_event_callbacks_t *callback=&self->callback;
    callback->on_cap = captureCallback;

    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(*cap_chan, callback, self));

    //More suitable to be in the class data, but  that makes retreving the base poointer using
    //container_of too difficult
    self->interface.startMonitoring=startMonitoring;
    self->interface.stopMonitoring=stopMonitoring;

    /*The queue is now exclusive to each capture object so that xQueueSendFromISR never fails because
    of conflict. There is however still  chance of failing if its queue is fill*/

    self->queue=xQueueCreate(QUEUE_LENGTH,sizeof(capture_event_data_t));
    if(self->queue==NULL)
        return ERR_CAPTURE_MEM_ALLOC;

    if(xTaskCreate(task_processCaptureQueue,"readQueue",4096,(void*) self,0,&self->capture_task)!=pdPASS)
        return ERR_CAPTURE_MEM_ALLOC;


    return 0;


   
}