
#include <stdint.h>
#include <string.h>

#include <esp_log.h>


#include "keypad_dev.h"

#include "my_timer.h"
#include "pool_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "scan_manager.h"
#include "probe_manager.h"
#include "keypad_button.h"


#define MAX_KEYPADS              CONFIG_MAX_KEYPADS
#define MAX_SIMULTANEOUS_KEYS    CONFIG_MAX_SIMULTANEOUS_KEYS
#define MAX_BUTTONS             (KPAD_MAX_COLS*KPAD_MAX_ROWS)

//These #defines are internal
//This first queue is the one that gets both scanner and timer events and passes to the corresponding button
#define MAX_INTERNAL_EVENT_QUEUE_ELEMENTS               100
//This queue gets the user evebts   
#define MAX_USER_EVENT_QUEUE_ELEMENTS                   50

//#define QUEUE_LENGTH                                100

static const char* TAG="keypad";

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))



#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


//Events for the MP queue
typedef enum{
    MP_EVENT_BUTTON_PRESS=0,
    MP_EVENT_TIMER_ELAPSED
}mp_event_t;



typedef struct{
    mp_event_t event;
    button_interface_t* button;          //Which button and which timer is causing this event
}mp_event_data_t;


/*typedef struct{
    key_event_t event;
    uint8_t button_id;           //Which button and which timer is causing this event
    uint32_t time_stamp;
}key_event_data_t;*/

//For The MP. Both the timer events and scan events will queue here
typedef struct queue_mp_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(mp_event_data_t)*MAX_INTERNAL_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_mp_event_handle_t;


typedef struct queue_user_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(keypad_event_data_t)*MAX_USER_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_user_event_handle_t;





typedef struct keypad_dev{
    uint8_t keymap[KPAD_MAX_ROWS][KPAD_MAX_COLS];
    key_press_mode_t mode;
    uint8_t row_gpios[KPAD_MAX_ROWS];               //GPIOs for the rows of keypad
    uint8_t total_rows; //The max rows is for the mem allocated, but this is the true value for the instance
    uint8_t col_gpio[KPAD_MAX_COLS];               //GPIOs for the cols of keypad
    uint8_t total_cols;//The max rows is for the mem allocated, but this is the true value for the instance
    scanner_interface_t* scanner;
    prober_t prober;
    keypad_interface_t interface;
    timer_interface_t* timers[MAX_SIMULTANEOUS_KEYS];   //array of pointers
    pool_alloc_interface_t* timer_pool; //This is the object passsed to button objects
    button_interface_t* button[MAX_BUTTONS];
    queue_mp_event_handle_t mp_event_queue;         //Multiple producer queue, scan and timer events
    queue_user_event_handle_t user_event_queue;     //Major events intimated to user
    TaskHandle_t mp_queue_task;
    TaskHandle_t user_queue_task;
    keypadCallback cb;
}keypad_dev_t;


//Pool of th objects, so that to have static allocation
typedef struct{
    keypad_dev_t array[MAX_KEYPADS];
    uint8_t allocated_count;
}keypad_pool_t;



static keypad_pool_t pool;


/*

    
    my_timer_t* timer=&self->timer;
    
    timerCreate(timer,"shared",NULL);

    QueueHandle_t queue_handle = self->queue_handle;


    

    queue_handle= xQueueCreateStatic(QUEUE_LENGTH,sizeof(queue_data_t));
    

}

*/

/// @brief Get one element(i.e a pool object out of pool array) of pool, and increment the count. Not thread safe
/// @return 
static keypad_dev_t* poolGet(){
    
    if(pool.allocated_count==MAX_KEYPADS)
        return NULL;
    
    keypad_dev_t* self=&pool.array[pool.allocated_count];
    pool.allocated_count++;
    return self;
}

//So it assumes that return is in the same order as get, so very simplisitic, also  not thread safe
static void poolReturn(){

    pool.allocated_count--;

}






static void copyUserParameters(keypad_dev_t* self,keypad_config_t* config){

    memcpy(self->col_gpio,config->col_gpios,sizeof(uint8_t)*config->total_cols);
    self->total_cols=config->total_cols;
    memcpy(self->row_gpios,config->row_gpios,sizeof(uint8_t)*config->total_rows);
    self->total_rows=config->total_rows;
    memcpy(self->keymap,config->keymap,sizeof(config->keymap));
    self->cb=config->cb;


}

static void scannerEventHandler(scanner_event_data_t* event_data,void* context){

    //Get the keypad_dev_t instance
    keypad_dev_t* self=(keypad_dev_t*)context;

    if(self->mp_event_queue.handle==NULL)
        return;

    uint8_t row=event_data->source_number;
    uint8_t col=event_data->line_number;
    uint8_t total_cols=self->total_cols;
    uint8_t button_index=((row*total_cols)+col);
    ESP_LOGI(TAG,"row %d, col %d, button index %d",row,col,button_index);
    button_interface_t* button=self->button[button_index];

    mp_event_data_t mp_event_data;
    mp_event_data.event=MP_EVENT_BUTTON_PRESS;
    mp_event_data.button=button;
    //ESP_LOGI(TAG,"before mp  sending");
    xQueueSend(self->mp_event_queue.handle,&mp_event_data,portMAX_DELAY);
    //ESP_LOGI(TAG,"after mp  sending");

    

}


static void timerEventHandler(timer_event_t event,void* creator_context,void* user_context){
    
    //keypad_button_t* self=(keypad_dev_t*) context;
    ESP_LOGI(TAG,"t hand");
    keypad_dev_t* keypad=(keypad_dev_t*) creator_context;
    mp_event_data_t event_data;
    button_interface_t* button=(button_interface_t*)user_context;
    event_data.button=button;
    event_data.event=MP_EVENT_TIMER_ELAPSED;         //Although different enum types , but ennum value is same i.e 1 for timer elapse

    QueueHandle_t queue=keypad->mp_event_queue.handle;
    //ESP_LOGI(TAG,"timer over");
    //Push the timer elapsed event with the context of the user (button) for which the timer elapsed.
    xQueueSend(queue,&event_data,portMAX_DELAY);

}




static void task_mp_queue(void* args){

    button_interface_t* button;
    keypad_dev_t* self=(keypad_dev_t*)args;
    QueueHandle_t queue_handle=(QueueHandle_t) self->mp_event_queue.handle;
    mp_event_data_t mp_event_data={0};
    
    while(1){
        if(xQueueReceive(queue_handle,&mp_event_data,portMAX_DELAY)==pdTRUE){

            //ESP_LOGI(TAG,"problem here %d",mp_event_data.event);
            button=mp_event_data.button;
            if(button==NULL || button->buttonEventInform==NULL)
                continue;
            //ESP_LOGI(TAG,"not so %d",mp_event_data.event);
            switch(mp_event_data.event){
                case MP_EVENT_BUTTON_PRESS:    button->buttonEventInform(button,BUTTON_STATE_EVENT_PRESSED); break;
                
                case MP_EVENT_TIMER_ELAPSED:    button->buttonEventInform(button,BUTTON_STATE_EVENT_TIMER_ELAPSED); break;

                //This is not useful or well though out. just written for the sake of writing a default statement
                default:    button->buttonEventInform(button,2); break;

            }
            
        }

    }

}

static void task_user_queue(void* args){
    keypad_dev_t* self=(keypad_dev_t*)args;
    QueueHandle_t queue_handle=(QueueHandle_t) self->user_event_queue.handle;
    keypad_event_data_t key_event_data={0};


    while(1){


        if(xQueueReceive(queue_handle,&key_event_data,portMAX_DELAY)==pdTRUE){
            ESP_LOGI(TAG,"sending key id %d",key_event_data.key_id);
            self->cb(key_event_data.event,&key_event_data);
            
        }


    }



}



static void buttonHandler(uint8_t button_index,button_event_data_t* evt,void* context){

    keypad_dev_t* self=(keypad_dev_t*) context;

    QueueHandle_t queue=self->user_event_queue.handle;

    keypad_event_data_t keypad_event_data={0};

    ESP_LOGI(TAG,"id %d, val %d",button_index,evt->button_id);
    keypad_event_data.event=evt->event;
    keypad_event_data.key_id=evt->button_id;    
    keypad_event_data.time_stamp=evt->timestamp;

    xQueueSend(queue,&keypad_event_data,portMAX_DELAY);
    
}

static esp_err_t configKeypadButtons(keypad_dev_t* self,uint8_t total_buttons){

    button_config_t config={0};
    config.scan_time_period=self->prober.time_period + 6000;
    ESP_LOGI(TAG,"timer period %"PRIu32,self->prober.time_period);
    config.cb=buttonHandler;
    config.context=(void*)self;
    config.timer_pool=self->timer_pool;
    
    
    uint8_t keymap_row_index=0;
    uint8_t keymap_col_index=0;
    uint8_t total_rows=self->total_rows;
    uint8_t total_cols=self->total_cols;
    for(uint8_t i=0;i<total_buttons;i++){
        
        config.button_index=i;  
        config.button_id=self->keymap[keymap_row_index][keymap_col_index];
        self->button[i]=keypadButtonCreate(&config);
        if(self->button[i]==NULL)
            return ESP_FAIL;
        keymap_col_index++;
        if(keymap_col_index==total_cols){
            keymap_col_index=0;
            keymap_row_index++;
        }

    }


    return ESP_OK;

}









//This same array is used both by prober and scanner
static uint32_t pulse_widths[]={2000,2400,2800,3200};        //Microseconds

static prober_t keypad_prober;
static esp_err_t configKeypadOutput(prober_t* self,uint8_t* output_gpio,uint8_t total_outputs){

    prober_config_t config={0};

    config.gpio_no=output_gpio;
    config.total_gpio=total_outputs;
    
    config.pulse_widths=pulse_widths;
    config.dead_time=1000;          //1000 microseconds
    config.time_period=14500;   //(2000+1000)+(2400+1000)+(2800+1000)+(3200+1000

    //initalizing the prober member of self. It needs to be changed to have internal static allocation inside scanner
    int ret=proberCreate(self,&config);

    return ret;

}

static esp_err_t configKeypadTimers(keypad_dev_t* self,uint8_t total_timers){

    char name[10];

    timer_interface_t** timer_array=self->timers;
    //void* context = (void*)self;//Now context is added by the user
    //                              and callback is addeed by keypad.
                                //so that keypbad callback knows to whcih user this call of callback belongs
    //void* context=(void*)t;
    for(uint8_t i=0;i<total_timers;i++){

        sprintf(name,"timer %d",i);
        timer_array[i]=timerCreate(name,timerEventHandler,self);
        if(timer_array[i]==NULL)
            return ESP_FAIL;

    }

    return ESP_OK;
}

static esp_err_t configTimerPool(keypad_dev_t* self,timer_interface_t** timers,uint8_t total_objects){

    //This call does not require any paramaters bcz Max objects that a pool can manage is the only requirement
    //which is set through Kconfig
    self->timer_pool=poolQueueCreate();
    if(self->timer_pool==NULL)
        return ESP_FAIL;

    //Now fill the pool
    for(uint8_t i=0;i<total_objects;i++){

        if(timers[i]==NULL)
            return ESP_FAIL;
        else
            self->timer_pool->poolFill(self->timer_pool,(void*)timers[i]);

    }

    return ESP_OK;

}

/// @brief Configure the scanner that reads the inputs of matrix keypad
/// The self pointer is double, so that context contains the address of scanner member and thus can extract keypad_dev_t self from context
/// @param input_gpio   The list of gpio associated
/// @param total_inputs Total gpios
/// @param total_outputs Total outputs of matrix keypad, each with different pwm signal and thus different signal
/// @return 
static esp_err_t configKeypadInput(keypad_dev_t* self,uint8_t* input_gpio,uint8_t total_inputs,uint8_t total_outputs){

    scanner_config_t config={0};
    scanner_interface_t * scn=self->scanner;


    config.gpio_no=input_gpio;
    config.total_gpio=total_inputs;
    config.total_signals=total_outputs;
    config.tolerance=100;       //microseconds so  0.1 milliseconds
    config.min_width=1500;      //1500 milliseconds. Pulse smaller than that will not be analyzed
    config.pwm_widths_array=pulse_widths;
    config.context=(void*)self; //This will later reterive the keypad_dev_t instance
    config.cb=scannerEventHandler;
    
    //Assigning the scanner member of the self
    self->scanner=scannerCreate(&config);

    if(self->scanner==NULL)
        return ESP_FAIL;

    return ESP_OK;

    
}




keypad_interface_t* keypadCreate(keypad_config_t* config){


    keypad_dev_t* self=poolGet();
    int ret;
    if(self==NULL || config==NULL)
        return NULL;
    
    
    copyUserParameters(self,config);
    //Here we are sending address bcz argument is double pointer and it is required to have the address of this pointer member inside the struct
    ret=configKeypadInput(self,self->col_gpio,self->total_cols,self->total_rows);
    ESP_LOGI(TAG,"scanner %d",ret);
    //This is already an instance member so argument is single pointer. No context is required for this since no callback
    ret=configKeypadOutput(&self->prober,self->row_gpios,self->total_rows);
    ESP_LOGI(TAG,"prober %d",ret);

    ret=configKeypadTimers(self,MAX_SIMULTANEOUS_KEYS);
    ESP_LOGI(TAG,"timer %d",ret);
    ret=configTimerPool(self,self->timers,MAX_SIMULTANEOUS_KEYS);
    ESP_LOGI(TAG,"timer pool %d",ret);
    ret=configKeypadButtons(self,MAX_BUTTONS);
    ESP_LOGI(TAG,"button %d",ret);

    self->mp_event_queue.handle=xQueueCreateStatic(MAX_INTERNAL_EVENT_QUEUE_ELEMENTS,sizeof(mp_event_data_t),self->mp_event_queue.buff,&self->mp_event_queue.queue_meta_data);

    self->user_event_queue.handle=xQueueCreateStatic(MAX_USER_EVENT_QUEUE_ELEMENTS,sizeof(keypad_event_data_t),self->user_event_queue.buff,&self->user_event_queue.queue_meta_data);

    ESP_LOGI(TAG, "Free heap before: %" PRIu32, esp_get_free_heap_size());
    xTaskCreate(task_mp_queue,"MP Queue Task",8192,(void*)self,5,&self->mp_queue_task);
    xTaskCreate(task_user_queue,"User Queue Task",4096,(void*)self,5,&self->user_queue_task);
    //Start scannning
    self->scanner->startScanning(self->scanner);
    //start probing
    self->prober.interface.start(&self->prober.interface);
    return &self->interface;
        
}

                                