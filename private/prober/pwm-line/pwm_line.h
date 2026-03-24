
#ifndef PWM_LINE_H

#define PWM_LINE_H




typedef enum {
    RUNNING,
    IDLE
}pwm_line_state_t;



typedef struct pwm_line_interface{
    void (*pwmStart)(struct pwm_line_interface* self);
    void (*pwmStop)(struct pwm_line_interface* self);   //Line High Idle state
    void (*pwmDestroy)(struct pwm_line_interface* self);
    void (*pwmChangeWidth)(struct pwm_line_interface* self,uint32_t pulse_width,uint32_t time_period);   //in microseconds

        
}pwm_line_interface_t;



typedef struct {
    uint32_t time_period;   // Time Period of wave in microseconds
    uint8_t timer_resolution;
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
    uint32_t time_period;   // Time Period of wave in microseconds
    int duty;                   //Private class data, required by driver in duty ticks (counter value)
    int hpoint;
    pwm_line_interface_t interface;

}pwm_line_t;


int pwmCreate(pwm_line_t* self,pwm_config_t*  config);
int pwmGetMaxChannels();
int pwmGetMaxTimerReolution();


#endif