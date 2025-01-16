
#ifndef MY_GPIO_H
#define MY_GPIO_H

#include "gpio_interface.h"



typedef struct my_gpio my_gpio_t;

typedef struct my_gpio{
    uint8_t number;
    void (*callback)(gpio_event_t);
    gpio_interface_t interface;
}my_gpio_t;





#endif
