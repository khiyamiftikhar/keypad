#ifndef PWM_LINE_INTERFACE_H
#define PWM_LINE_INTERFACE_H



#include "stdint.h"
#include "stddef.h"



typedef struct pwm_line_interface{
    void (*pwmStart)(struct pwm_line_interface* self);
    void (*pwmStop)(struct pwm_line_interface* self);   //Line High Idle state

        
}pwm_line_interface_t;

#endif
