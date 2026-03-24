#ifndef SCANNER_H
#define SCANNER_H





#define ERR_SCANNER_BASE                    0
#define ERR_SCANNER_INVALID_MEM             (ERR_SCANNER_BASE-1)
#define ERR_SCANNER_MEM_ALLOC               (ERR_SCANNER_BASE-2)



//Simply relaying what it receives from pulse decoder
//Exact same members
typedef struct scanner_event_data{

    uint8_t source_number;          //based on pwm width array
    uint8_t line_number;            //1 , 2 3 instead of gpio number 222,34 etc

}scanner_event_data_t;



typedef void (*scannerCallback)(scanner_event_data_t* event_data,void* context);

typedef struct scanner_interface{
   // void (*addCaptureLine)(scanner_interface_t* self,pwm_capture_t* line);
    int (*startScanning)(struct scanner_interface* self);
    int (*stopScanning)(struct scanner_interface* self);
    int (*destroy)(struct scanner_interface* self);
    /*So the callback does not have self referencing because it will be defined by the user of this, 
        and purpose of self referencing is to access the private members of an instance which are
        different for each because they are also at different unique addresses
    */
    
}scanner_interface_t;










typedef struct scanner_config{
    uint8_t total_gpio; 
    //TaskHandle_t scanner_task;
    ///QueueHandle_t queue;
    uint8_t* gpio_no;
    uint8_t total_signals;
    uint32_t* pwm_widths_array;
    uint32_t tolerance;
    scannerCallback cb;
    void* context;
}scanner_config_t;






//not used bcz struct shard in header
//size_t scannerGetSize(uint8_t total_gpio, uint8_t total_signals);
int scannerCreate(scanner_config_t* config,scanner_interface_t** scanner);


#endif