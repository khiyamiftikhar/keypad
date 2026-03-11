#ifndef KEYPAD_BUTTON_H
#define KEYPAD_BUTTON_H

#include "stdint.h"
#include "pool_alloc_interface.h"


#define         ERR_BUTTON_BASE                 0
//#define         ERR_BUTTON_INVALID_MEM          ( ERR_BUTTON_BASE-1 )


//Events of the GPIO related to the button. incoming events
typedef enum{
            BUTTON_STATE_EVENT_PRESSED=0,
            //BUTTON_EVENT_RELASED, This event will never come, that is why timer is used
            BUTTON_STATE_EVENT_TIMER_ELAPSED
            }button_state_update_event_t;


//button events reported to the user
typedef enum{
    BUTTON_EVENT_PRESSED=0,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_PRESSED_LONG,
    BUTTON_EVENT_PRESSED_REPEAT
}button_event_t;


typedef struct button_event_data{
    uint8_t button_id;
    button_event_t event;
    uint32_t timestamp;             
}button_event_data_t;

typedef void (*buttonCallBack)(uint8_t button_index,button_event_data_t* evt,void* context);


typedef struct button_interface{

    int (*buttonEventInform)(struct button_interface* self,button_state_update_event_t evt);
    buttonCallBack cb;
    
}button_interface_t;



typedef struct button_config{
    uint8_t button_index;                          //index in the button array
    uint8_t button_id;                             //value or name
    pool_alloc_interface_t* timer_pool;           //to allocate and deallocate the timer object
    uint32_t scan_time_period;     //Button if pressed, its value must come in this time duration, if no presseed event comes, then button is released
    buttonCallBack cb;          //Callback to call  on a major event
    void* context;  //To know which use (keypad) instance it belongs to
}button_config_t;

button_interface_t* keypadButtonCreate(button_config_t* config);


#endif