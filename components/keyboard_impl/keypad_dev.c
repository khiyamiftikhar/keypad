
#include <stdint.h>
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
#define INTERNAL_EVENT_QUEUE_ELEMENTS               100
//This queue gets the user evebts   
#define USER_EVENT_QUEUE_ELEMENTS                   50

//#define QUEUE_LENGTH                                100

static const char* TAG="keypad";

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))



#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


//Events for the MP queue
typedef enum{
    BUTTON_PRESS_EVENT=0,
    TIMER_ELAPSE_EVENT
}mp_event_t;


//For The MP. Both the timer events and scan events will queue here
typedef struct queue_mp_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(void*)*INTERNAL_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_mp_event_handle_t;


typedef struct queue_user_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(void*)*USER_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
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


}

static void callbackForScanner(scanner_event_data_t* event_data,void* context){

    //Get the keypad_dev_t instance
    keypad_dev_t* self=(keypad_dev_t*)context;
    

}


static void buttonHandler(uint8_t button_id,button_event_data_t* evt,void* context){

    keypad_dev_t* self=(keypad_dev_t*) context;

}

static esp_err_t configKeypadButtons(keypad_dev_t* self,uint8_t total_buttons){

    button_config_t config={0};
    config.scan_time_period=self->prober.time_period;
    config.cb=buttonHandler;
    config.context=(void*)self;
    config.timer_pool=self->timer_pool;
    
    
    for(uint8_t i=0;i<total_buttons;i++){
        config.button_index=i;    
        self->button[i]=keypadButtonCreate(&config);
        if(self->button==NULL)
            return ESP_FAIL;
    }


    return ESP_OK;

}









//This same array is used both by prober and scanner
static uint32_t pulse_widths={2000,2400,2800,3200};        //Microseconds

static prober_t keypad_prober;
static esp_err_t configKeypadOutput(prober_t* self,uint8_t* output_gpio,uint8_t total_outputs){

    prober_config_t config={0};

    config.gpio_no=output_gpio;
    config.total_gpio=total_outputs;
    
    config.pulse_widths=pulse_widths;
    config.dead_time=1000;          //1000 microseconds
    config.time_period=14500;   //(2000+1000)+(2400+1000)+(2800+1000)+(3200+1000

    //initalizing the prober member of self. It needs to be changed to have internal static allocation inside scanner
    int ret=proberCreate(&self,&config);

    return ret;

}

static void timerCallback(timer_event_t event,void* context){
    
    keypad_dev_t* self=(keypad_dev_t*) context;


}

static esp_err_t configKeypadTimers(keypad_dev_t* self,uint8_t total_timers){

    char name[10];

    timer_interface_t** timer_array=self->timers;
    void* context = (void*)self;
    //void* context=(void*)t;
    for(uint8_t i=0;i<total_timers;i++){

        sprintf(name,"timer %d",i);
        timer_array[i]=timerCreate(name,timerCallback,context);
        if(timer_array[i]==NULL)
            return ESP_FAIL;

    }

    return ESP_OK;
}

static esp_err_t configTimerPool(pool_alloc_interface_t* pool,timer_interface_t** timers,uint8_t total_objects){

    //This call does not require any paramaters bcz Max objects that a pool can manage is the only requirement
    //which is set through Kconfig
    pool=poolQueueCreate();
    if(pool==NULL)
        return ESP_FAIL;

    //Now fill the pool
    for(uint8_t i=0;i<total_objects;i++){

        if(timers[i]==NULL)
            return ESP_FAIL;
        else
            pool->poolFill(pool,(void*)timers[i]);

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
    config.cb=callbackForScanner;
    
    //Assigning the scanner member of the self
    scn=scannerCreate(&config);

    if(scn==NULL)
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

    
        
}

                                