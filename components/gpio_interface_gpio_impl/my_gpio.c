
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "gpio_interface.h"
#include "my_gpio.h"

static const char* TAG="my gpio";
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


typedef struct isr_data{
    void (*callback)(gpio_event_t);
    void* context;
}isr_data_t;


static void isr_handler(void* arg){

    my_gpio_t* my_gpio=(my_gpio_t*)arg;

    uint8_t level=gpio_get_level(my_gpio->number);

    gpio_event_t event=level?NEGEDGE:POSEDGE;
    my_gpio->callback(event);
}


int gpioMode(gpio_interface_t* self, gpio_mode_t mode){
   
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);
    esp_err_t err_check=0;
    gpio_config_t io_config;

    my_gpio->mode=mode;

    switch (mode){
        case GPIO_MODE_INPUT     :            io_config.mode=GPIO_MODE_INPUT;   io_config.pull_up_en=0;    
                                    
                                    break;
        case GPIO_MODE_INPUT_PULLUP   :       io_config.mode=GPIO_MODE_INPUT;   io_config.pull_up_en=1;    
                                    
                                    break;

        case GPIO_MODE_OUTPUT    :            io_config.mode=GPIO_MODE_OUTPUT;  io_config.pull_up_en=0;
                                    
                                    break;
        case GPIO_MODE_INPUT_OUTPUT    :     io_config.mode=GPIO_MODE_INPUT_OUTPUT;        io_config.pull_up_en=0;    
                                    break;
                                    
    
        default             :      io_config.mode=GPIO_MODE_INPUT;   io_config.pull_up_en=0;    break;
    }
    

    ESP_LOGI(TAG,"mode %d",mode);
    io_config.pin_bit_mask=(1ULL<<my_gpio->number);
      
    
    io_config.intr_type=GPIO_INTR_DISABLE;
    io_config.pull_down_en=0;

    //ESP_LOGI(TAG,"inttype, %d",io_config.intr_type);    
    err_check=gpio_config(&io_config);

    return err_check;

    
    
    
}
int digitalWrite(gpio_interface_t* self,uint8_t value){

    if(self==NULL)
        return -1;
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);

    gpio_set_level(my_gpio->number,value);

    return 0;
}

int digitalRead(gpio_interface_t* self){
    
        if(self==NULL)
            return -1;
        my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);

        return gpio_get_level(my_gpio->number);
}


int attachInterrupt(gpio_interface_t* self, void (*callback)(gpio_event_t), interrupt_mode mode){  // Use GPIO number

    if(self==NULL || callback==NULL)
        return -1;
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);
    my_gpio->callback=callback;

    switch(mode){
        case GPIO_INTERRUPT_MODE_RISING:    mode=GPIO_INTR_POSEDGE; break;
        case GPIO_INTERRUPT_MODE_FALLING:   mode=GPIO_INTR_NEGEDGE; break;
        case GPIO_INTERRUPT_MODE_CHANGE:    mode=GPIO_INTR_ANYEDGE; break;
        default:        mode=GPIO_INTR_ANYEDGE;
    }


    esp_err_t ret= gpio_set_intr_type(my_gpio->number, mode);
    if(ret!=ESP_OK)
        return ESP_FAIL;

   

    gpio_install_isr_service(0);        //Flag 0 is default


    ret=gpio_isr_handler_add((gpio_num_t) my_gpio->number, isr_handler,(void*) my_gpio);
    return ret;

}



int enableInterrupt(gpio_interface_t* self){
    
    if(self==NULL)
        return -1;
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);
    
    esp_err_t ret= gpio_intr_enable((gpio_num_t) my_gpio->number);
    return ret;

}


int disableInterrupt(gpio_interface_t* self){
    
    if(self==NULL)
        return -1;
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);
    
    esp_err_t ret= gpio_intr_disable((gpio_num_t) my_gpio->number);
    return ret;
}

int gpioCreate(my_gpio_t* my_gpio,uint8_t number){

    if(my_gpio==NULL)
        return -1;
    
    my_gpio->number=number;
    my_gpio->mode=GPIO_MODE_INPUT;
    my_gpio->interface.attachInterrupt=attachInterrupt;
    my_gpio->interface.digitalRead=digitalRead;
    my_gpio->interface.digitalWrite=digitalWrite;
    my_gpio->interface.enableInterrupt=enableInterrupt;
    my_gpio->interface.disableInterrupt=disableInterrupt;
    my_gpio->interface.gpioMode=gpioMode;

    return 0;

}