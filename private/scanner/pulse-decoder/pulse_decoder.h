#ifndef PULSE_DECODER_H

#define PULSE_DECODER_H

#include "stdint.h"
#include "stddef.h"
//#include "capture_interface.h"
//#include "capture_event_data.h"

#define     ERR_CAPTURE_BASE                     0
#define     ERR_CAPTURE_MEM_ALLOC               (ERR_CAPTURE_BASE-1)
#define     ERR_CAPTURE_CAP_UNIT_EXCEED         (ERR_CAPTURE_BASE-2)    //At most 6 cap units
#define     ERR_CAPTURE_INVALID_ARGS            (ERR_CAPTURE_BASE-3)
#define     ERR_CAPTURE_UNREGISTERD_PULSE_WIDTH (ERR_CAPTURE_BASE-4)


typedef struct pulse_decoder_event_data{

    uint8_t source_number;          //based on pwm width array
    uint8_t line_number;            //1 , 2 3 instead of gpio number 222,34 etc

}pulse_decoder_event_data_t;






typedef void (*pulseDecoderEventCallback)(pulse_decoder_event_data_t* event_data,void* context);
//typedef struct ir_monitor ir_monitor_t;
/// @brief The user supplies the total PWM signals it will monitor. For example for a 4*4 keypad, it will
///         be 4 signals coming coming from 4 lines to this single input line when button pressed
/// @param total_signals 
/// @return Total size required for allocation
//size_t monitorGetSize(uint8_t total_signals);



typedef struct pulse_decoder_interface{
    int (*startMonitoring)(struct pulse_decoder_interface* self);
    int (*stopMonitoring)(struct pulse_decoder_interface* self);
    int (*destroy)(struct pulse_decoder_interface* self);
}pulse_decoder_interface_t;



typedef struct{
    uint8_t gpio_num;
    uint32_t* pulse_widths_us;
    uint8_t total_signals;          //length of pulse_width_us array
    uint32_t tolerance_us;         //+/- value   
    pulseDecoderEventCallback cb;
    void* context;
}pulse_decoder_config_t;


esp_err_t pulseDecoderCreate(pulse_decoder_config_t    *config,
                              pulse_decoder_interface_t **out_if);















#endif
