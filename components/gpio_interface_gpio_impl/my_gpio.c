
#include <stdio.h>
#include "driver/gpio.h"
#include "gpio_interface.h"
#include "my_gpio.h"


typedef struct isr_data{
    void (*callback)(gpio_event_t);
    void* context;
}isr_data_t;


static void isr_handler(void* arg){



}


void gpioMode(gpio_interface_t* self, gpio_mode mode){
   
    my_gpio_t* my_gpio=container_of(self,my_gpio_t,interface);
    esp_err_t err_check=0;
    gpio_config_t io_config;

    switch (mode){
        case INPUT     :           io_config.mode=GPIO_MODE_INPUT;         io_config.pull_up_en=0;    break;
        case INPUT_PULLUP   :      io_config.mode=GPIO_MODE_INPUT;         io_config.pull_up_en=1;    break;
        case OUTPUT    :           io_config.mode=GPIO_MODE_OUTPUT;        io_config.pull_up_en=0;    break;
        default             :      io_config.mode=GPIO_MODE_INPUT;         io_config.pull_up_en=0;    break;
    }
    
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

    gpio_set_level()



}
uint8_t (*digitalRead)(gpio_interface_t* self, uint8_t gpio);  // Use GPIO number


void attachInterrupt(gpio_interface_t* self, uint8_t gpio, void (*callback)(gpio_event_t), interrupt_mode mode,void* context){  // Use GPIO number


    switch(mode){
        case RISING:    mode=GPIO_INTR_POSEDGE;
        case FALLING:   mode=GPIO_INTR_NEGEDGE;
        case CHANGE:    mode=GPIO_INTR_ANYEDGE;
        default:        mode=GPIO_INTR_ANYEDGE;


    esp_err_t ret= gpio_set_intr_type((gpio_num_t) gpio, mode);
    if(ret!=ESP_OK)
        return ESP_FAIL;

    ret= gpio_intr_enable((gpio_num_t) gpio);

    gpio_install_isr_service(0);        //Flag 0 is default


    isr_data_t* isr_data=  gpio_isr_handler_add((gpio_num_t) gpio, isr_handler,(void*) args);

}



void (*detachInterrupt)(gpio_interface_t* self, uint8_t gpio);  // Use GPIO number


*/
