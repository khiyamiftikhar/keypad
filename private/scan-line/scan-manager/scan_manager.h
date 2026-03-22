#ifndef SCANNER_H
#define SCANNER_H

#include "capture_event_data.h"



#define ERR_SCANNER_BASE                    0
#define ERR_SCANNER_INVALID_MEM             (ERR_SCANNER_BASE-1)
#define ERR_SCANNER_MEM_ALLOC               (ERR_SCANNER_BASE-2)


typedef struct scanner_interface{
   // void (*addCaptureLine)(scanner_interface_t* self,pwm_capture_t* line);
    int (*startScanning)(struct scanner_interface* self);
    int (*stopScanning)(struct scanner_interface* self);
    /*So the callback does not have self referencing because it will be defined by the user of this, 
        and purpose of self referencing is to access the private members of an instance which are
        different for each because they are also at different unique addresses
    */
    void (*callback)(scanner_event_data_t* event_data,void* context);
}scanner_interface_t;






//This is defined separately just so that the scannerCreate callback parameter is simple
//Scanner object creation results in a pointer
typedef void (*callbackForScanner)(scanner_event_data_t* event_data,void* context);




typedef struct scanner_config{
    uint8_t total_gpio; 
    //TaskHandle_t scanner_task;
    ///QueueHandle_t queue;
    uint8_t* gpio_no;
    uint8_t total_signals;
    uint32_t* pwm_widths_array;
    uint32_t tolerance;
    uint32_t min_width;
    callbackForScanner cb;
    void* context;
}scanner_config_t;






//not used bcz struct shard in header
//size_t scannerGetSize(uint8_t total_gpio, uint8_t total_signals);
scanner_interface_t* scannerCreate(scanner_config_t* config);


#endif