#ifndef CAPTURE_INTERFACE_H
#define CAPTURE_INTERFACE_H
/*An interface is designed keeping it mind that it must be open to be implemented using any 
peripheral. So this particulat can be implementd using MCPWM,RMT*/


#include <stdint.h>


/*
typedef struct monitor_config{
    uint8_t gpio;
    uint32_t tolerance;             //+/- microseconds
}monitor_config_t;



typedef struct pulse_config{
    uint8_t pulse_id;
    uint32_t pulse_width;
}pulse_config_t;
*/

typedef struct capture_interface{
    
    
    
    /*The object will not call the callback if the pulse width is lower than this*/
    int(*setGPIO)(struct capture_interface* self,uint8_t gpio_num);
    int(*setMinWidth)(struct capture_interface* self,uint32_t min_width);
    int(*registerCallback)(struct capture_interface* self,void(*callback)(uint32_t pulse_width));
    int (*startMonitoring)(struct capture_interface* self);
    int (*stopMonitoring)(struct capture_interface* self);

}capture_interface_t;


#endif