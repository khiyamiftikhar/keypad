#ifndef MY_GPIO_H
#define MY_GPIO_H

#include "gpio_interface.h"

typedef struct gpio_context{
    uint8_t gpio_num;
    void (*callback)(gpio_event_t)
    
}gpio_context_t;


typedef struct my_gpio my_gpio_t;

struct my_gpio{
    gpio_context_t context;
    gpio_interface_t interface;
}





#endif