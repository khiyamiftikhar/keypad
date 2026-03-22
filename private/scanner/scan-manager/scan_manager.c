/*

The design not well though the multiple instances of scan manager. The ESPIDF internally keeps record
oof the channels assigned and there is no explicit way to specify the channel. Only timer group can be
told. There are two groups and each has 3 channels. When API call is called with timer group 0, one by
one each vacant channels will be assigned. So this implementation as of yet does not maintain record 
as how many channels have been used and which group to use now
capture channels
*/


#include <string.h>
#include "esp_log.h"
#include "pwm_capture.h"
#include "pwm_capture.h"
#include "capture_event_data.h"
#include "scan_manager.h"



static const char* TAG = "scan manager";
#define         MAX_CHANNELS                6
#define         MAX_CHANNELS_PER_UNIT       3
#define         MAX_SCANNERS                2   //Maximum number of scanners, so two keypads at max
#define         QUEUE_LENGTH                50
#define         QUEUE_WAIT_TIME             5  //ms             
#define         QUEUE_WAIT_TICKS            (pdMS_TO_TICKS(QUEUE_WAIT_TIME))



#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



//This needs to be modified with void pointers because to avoid includong pwm_capture.h  here
struct scanner{
    uint8_t total_lines;
    pwm_capture_class_data_t* class_data;    //data share among all the members
    pwm_capture_t* list;                    //Initialized internally
    QueueHandle_t queue;                    //Separate Queue for each capture unit bcz ISR queue API doesn't wait and fails immediately
    TaskHandle_t capture_task;              //Corresponding task
    scanner_interface_t interface;
    void* context;
};

typedef struct scanner scanner_t;

/*This is the static pool from where users will get the scanner objects, so no dynamic alloc*/
typedef struct scanner_pool{

    scanner_t sc[MAX_SCANNERS];
    uint8_t count;
}scanner_pool_t;


static scanner_pool_t pool;
//typedef struct pulse_scanner scanner_t;




/// @brief This task receives data from multiple capture objects
/// @param args 
static void task_processScannerQueue(void* args){


    scanner_t* self=(scanner_t*) args;

    QueueHandle_t queue=self->queue;

    scanner_event_data_t scn_evt_data;
    callbackForScanner cb=self->interface.callback;
    while(1){
        if(xQueueReceive(queue,&scn_evt_data,portMAX_DELAY)==pdTRUE){
            cb(&scn_evt_data,self->context);

        }
    }

}





static void callback(scanner_event_data_t* data,void* context){

    scanner_t* self=(scanner_t*) context;
    QueueHandle_t queue=self->queue;

    xQueueSend(queue,data,QUEUE_WAIT_TICKS);
}

static int startScanning(scanner_interface_t* self){

    scanner_t* scanner = container_of(self,scanner_t,interface);


    uint8_t total_lines=scanner->total_lines;
    ESP_LOGI(TAG,"got u %d",total_lines);


    
    for(uint8_t i=0; i<total_lines ; i++){

        scanner->list->interface.startMonitoring(&scanner->list[i].interface);

    }
    
    return 0;    
}



/// @brief Get one element of pool, and increment the count. Not thread safe
/// @return 
static scanner_t* poolGet(){
    
    if(pool.count==MAX_SCANNERS)
        return NULL;
    
    scanner_t* self=&pool.sc[pool.count];
        pool.count++;
    return self;

}

static void poolReturn(){

    pool.count--;

}



scanner_interface_t* scannerCreate(scanner_config_t* config){


    //All scanners already allocated

    scanner_t* self=poolGet();

    
    if(self==NULL || config==NULL)
        return NULL;

    //Get one  scanner_t element from pool
    
    
    uint8_t total_gpio=config->total_gpio; 
    uint8_t* gpio_no=config->gpio_no;
    uint8_t total_signals=config->total_signals;
    uint32_t* pwm_widths_array=config->pwm_widths_array;
    uint32_t tolerance=config->tolerance;
    uint32_t min_width=config->min_width;
    callbackForScanner cb=config->cb;



    
    //Assign the memory to the pointers, list member gets the memory after the struct
    //self->list=(pwm_capture_t*)((uint8_t*)self + sizeof(scanner_t));

    self->list=(pwm_capture_t*) malloc(sizeof(pwm_capture_t)*total_gpio);
    //The class data gets the memory  after the list
    pwm_capture_class_data_t* class_data=(pwm_capture_class_data_t*) malloc(sizeof(pwm_capture_class_data_t));



    self->total_lines=total_gpio;
    self->interface.callback=cb;
    self->interface.startScanning=startScanning;
    self->context=config->context;
    
    
    
    //Class data init of capture object
    //Restart if error
    int ret=0;
    /*The callback paramter passed here is the intermediate callback defined in this file
    The callback paramter received in this function is the user callback called by this callback
    defined in this file
    */
    ESP_ERROR_CHECK(captureClassDataInit(class_data,min_width, tolerance,pwm_widths_array,total_gpio,total_signals, callback,(void*)self));

    self->class_data=class_data;
    /*
    if(ret!=0)
        return ret;
    */

    //Create all the instances of the capture objects now

    for(uint8_t i=0;i<total_gpio;i++){
        
        //The captureCreate function requires that
        memset(&self->list[i],0,sizeof(pwm_capture_t));
        //Assign each member of the list which is pwm_capture_t type
        self->list[i].class_data=class_data;
        self->list[i].index=i;                  //So that send index number instead of gpio number in callback
        ESP_ERROR_CHECK(captureCreate(&self->list[i],class_data,gpio_no[i]));
        self->class_data->count++;
        
        /*
        if(ret!=0)
            return ret;*/
    }


    self->queue=xQueueCreate(QUEUE_LENGTH,sizeof(scanner_event_data_t));
    if(self->queue==NULL){
        poolReturn();           //Give the element back. Just decrements the pointer, bcz actual element is never moved
        return NULL;
    }

    if(xTaskCreate(task_processScannerQueue,"merge_captures",4096,(void*) self,0,&self->capture_task)!=pdPASS){
        poolReturn();
        return NULL;
    }


 


    return &(self->interface);

}



