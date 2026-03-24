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
#include "esp_err.h"
#include "pulse_decoder.h"
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
    uint8_t* gpio_no;
    uint8_t total_gpio;
    uint32_t* pwm_widths_array; //just pointers here , The pulse decoder object already has space for mem
    uint8_t total_signals;
    uint32_t tolerance;
    //QueueHandle_t queue;                    //Separate Queue for each capture unit bcz ISR queue API doesn't wait and fails immediately
    //TaskHandle_t capture_task;              //Corresponding task
    scannerCallback cb;
    scanner_interface_t interface;
    void* context;
    pulse_decoder_interface_t* pulse_decoder[];   //Flexible array, put at end as required
};

typedef struct scanner scanner_t;




//typedef struct pulse_scanner scanner_t;




/*
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

*/



static void pulseDecoderEventHandler(pulse_decoder_event_data_t* data,void* context){

    scanner_t* self=(scanner_t*) context;
    //QueueHandle_t queue=self->queue;

    scanner_event_data_t  evt_data;
    evt_data.line_number=data->line_number;
    evt_data.source_number= data->source_number;

    self->cb(&evt_data,self->context);
    //xQueueSend(queue,data,QUEUE_WAIT_TICKS);
}

static int startScanning(scanner_interface_t* self){

    scanner_t* scanner = container_of(self,scanner_t,interface);


    uint8_t total_lines=scanner->total_gpio;
    ESP_LOGI(TAG,"got u %d",total_lines);


    
    for(uint8_t i=0; i<total_lines ; i++){

        scanner->pulse_decoder[i]->startMonitoring(scanner->pulse_decoder[i]);
        
    }
    
    return 0;    
}




static int stopScanning(scanner_interface_t* self){

    scanner_t* scanner = container_of(self,scanner_t,interface);


    uint8_t total_lines=scanner->total_gpio;
    ESP_LOGI(TAG,"got u %d",total_lines);


    
    for(uint8_t i=0; i<total_lines ; i++){

        scanner->pulse_decoder[i]->stopMonitoring(scanner->pulse_decoder[i]);
        
    }
    
    return 0;    
}


/// @brief Get one element of pool, and increment the count. Not thread safe
/// @return 

esp_err_t scannerCreate(scanner_config_t* config,scanner_interface_t** scanner){


    //All scanners already allocated
    if(config==NULL)
        return ESP_FAIL;

    scanner_t* self=(scanner_t*) malloc(sizeof(scanner_t)+sizeof(pulse_decoder_interface_t*)*config->total_gpio);

    
    if(self==NULL)
        return ESP_FAIL;

    //Get one  scanner_t element from pool
    
    
    self->total_gpio=config->total_gpio; 
    self->gpio_no=config->gpio_no;
    self->total_signals=config->total_signals;
    self->pwm_widths_array=config->pwm_widths_array;
    self->tolerance=config->tolerance;
    self->cb=config->cb;
    self->context=config->context;



   
    pulse_decoder_config_t decoder_config={0};
    
    decoder_config.context = (void*)self;
    decoder_config.tolerance_us=config->tolerance;
    decoder_config.total_signals=config->total_signals;
    decoder_config.pulse_widths_us=config->pwm_widths_array;
    
    for(uint8_t i=0;i<config->total_gpio;i++){
        
        
        decoder_config.gpio_num = config->gpio_no[i];
        decoder_config.line_num = i;
        
        esp_err_t ret=pulseDecoderCreate(&decoder_config,&self->pulse_decoder[i]);
        
        if(ret!=ESP_OK){
            for(uint8_t j=i-1; j>=0;j--){
                self->pulse_decoder[i]->destroy(self->pulse_decoder[i]);
               
            }

            free(self);
            return ESP_FAIL;

        }

    }


    /*
    self->queue=xQueueCreate(QUEUE_LENGTH,sizeof(scanner_event_data_t));
    if(self->queue==NULL){
        poolReturn();           //Give the element back. Just decrements the pointer, bcz actual element is never moved
        return NULL;
    }

    if(xTaskCreate(task_processScannerQueue,"merge_captures",4096,(void*) self,0,&self->capture_task)!=pdPASS){
        poolReturn();
        return NULL;
    }*/


 
    self->interface.startScanning=startScanning;
    self->interface.stopScanning=stopScanning;


    *scanner =&(self->interface);

    return ESP_OK;

}



