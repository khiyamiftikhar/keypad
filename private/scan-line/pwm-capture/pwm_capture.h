#ifndef PWM_CAPTURE_H

#define PWM_CAPTURE_H

#include "stdint.h"
#include "stddef.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/mcpwm_cap.h"
#include "capture_interface.h"
#include "capture_event_data.h"

#define     ERR_CAPTURE_BASE                     0
#define     ERR_CAPTURE_MEM_ALLOC               (ERR_CAPTURE_BASE-1)
#define     ERR_CAPTURE_CAP_UNIT_EXCEED         (ERR_CAPTURE_BASE-2)    //At most 6 cap units
#define     ERR_CAPTURE_INVALID_ARGS            (ERR_CAPTURE_BASE-3)
#define     ERR_CAPTURE_UNREGISTERD_PULSE_WIDTH (ERR_CAPTURE_BASE-4)


//typedef struct ir_monitor ir_monitor_t;

typedef struct cap_timer{
    mcpwm_cap_timer_handle_t cap_timer;
    mcpwm_capture_timer_config_t cap_conf;
}cap_timer_t;


typedef struct pwm_capture_class_data{
    
    cap_timer_t* timer;             //A single timer belongs to a group. MCPWM has two groups each having three capture units. This is an array of timers depending upon total capture objects created. So if there are 4 objects then there are two timers, one for first 3 belonging to same group and 2nd for the 4th
    uint32_t total_signals;         //Total signals (different width PWM) per line. Same for all
    uint32_t* pulse_widths;         //Width that differentiate different pulses.
    uint32_t  tolerance;            //The +/- tolerance range when comparing the received signal from the standard widths
    uint8_t count;                  //Count of how many objects have been created, to select capture group. One MCPWM group has 3
    uint32_t min_width;             //Min width of pulse that qualifies it to be valid for comparison
    void (*callback)(scanner_event_data_t* evt_data,void* context);   //This is the handler inside the scan manager, context added so that scanner object is passed here, so that in callback queue object of scan manager can be accesed
    //bool fuse;              //Once the variables are set, the fuse is also set, giving 'final' feature

    void* context;              //The scan manager object is referred here, so that on callback it can retrive
                                //Because the callback handler inn scan manageer must call the queue member                    
}pwm_capture_class_data_t;




//This is defined separately just so that the scannerCreate callback parameter is simple
//Name changed bcz cannot use the name callback in multiple files
typedef void (*callbackForCapture)(scanner_event_data_t* event_data,void* context);


//This is the Capture object, The class data pointer is one of its member which is shared among multiple objects

typedef struct pwm_capture{

    pwm_capture_class_data_t* class_data;       //only one instance. All instances will share it.
    uint8_t gpio_num;
    uint8_t index;                          //its index number in the array of gpio;
    QueueHandle_t queue;                    //Separate Queue for each capture unit bcz ISR queue API doesn't wait and fails immediately
    TaskHandle_t capture_task;              //Corresponding task
    mcpwm_cap_timer_handle_t cap_timer;         //different for different instances depending upon their mcpwm group
    mcpwm_cap_channel_handle_t cap_chan;
    mcpwm_capture_channel_config_t cap_ch_conf;
    mcpwm_capture_event_callbacks_t callback;
    capture_interface_t interface;  /*Although class data was a better place but that makes it too difficult
                                      to retreive the pointer using container_of  */
    uint32_t time_stamp;           //To store positive edge tick and then time value after negedge arrives
}pwm_capture_t;




/// @brief The user supplies the total PWM signals it will monitor. For example for a 4*4 keypad, it will
///         be 4 signals coming coming from 4 lines to this single input line when button pressed
/// @param total_signals 
/// @return Total size required for allocation
//size_t monitorGetSize(uint8_t total_signals);

int captureClassDataInit(pwm_capture_class_data_t* self,uint32_t min_width, uint32_t tolerance,uint32_t* pwm_widths_array,uint8_t total_gpio, uint8_t total_signals, callbackForCapture cb, void* context);
int captureCreate(pwm_capture_t* self, pwm_capture_class_data_t* class_data, uint8_t gpio);














#endif
