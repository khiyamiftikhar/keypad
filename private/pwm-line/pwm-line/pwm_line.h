
#ifndef PWM_LINE_H

#define PWM_LINE_H

#include "pwm_line_interface.h"


typedef enum {
    RUNNING,
    IDLE
}pwm_line_state_t;


typedef struct {
    uint32_t time_period;   // Time Period of wave in microseconds
    uint32_t pulse_width;   // Corresponds to duty cycle, in microseconds
    uint16_t phase;         //Degrees e.g 90, 180
    uint32_t dead_time;     //in microseconds Before this pulse, added to phase, so an addon on phase
    uint8_t gpio;           // GPIO pin number
    uint8_t channel_number; //In the group 1st , 2nd , third etc. seems useless
    void* context;      //For any data structure created by the ESPIDF driver
} pwm_config_t;


typedef struct pwm_line{

    //pwm_config_t  config;
    uint8_t channel_number;
    uint8_t gpio_number;
    int duty;                   //Private class data, required by driver in duty ticks (counter value)
    int hpoint;
    pwm_line_interface_t interface;

}pwm_line_t;


int pwmCreate(pwm_line_t* self,pwm_config_t*  config);


#endif