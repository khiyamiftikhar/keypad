/*This is the Timer interface reqquired by the keypad*/

#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H
#include <stdint.h>


typedef enum timer_event_id{
    TIMER_LONG_PRESS_EVENT,
    TIMER_REPEAT_EVENT,
    TIMER_STARTED,
    TIMER_STOPPED
}timer_event_id_t;


typedef struct timer_event{
    timer_event_id_t event_type;
}timer_event_t;



struct timer_interface{
    int (*timerSetLongPressInterval)(struct timer_interface*,uint64_t long_press_interval);   
    int (*timerSetRepeatInterval)(struct timer_interface*,uint64_t repeat_interval);   
    int (*timerLongPressStart)(struct timer_interface*);
    int (*timerLongPressStop)(struct timer_interface*);
    int (*timerLongPressReset)(struct timer_interface*);
    int (*timerRepeatStart)(struct timer_interface*);
    int (*timerRepeatStop)(struct timer_interface*);
    uint64_t (*timerGetCurrentTime)();
    int (*timerRegisterCallback)(struct timer_interface*,void(*callback)(timer_event_t*));
};

typedef struct timer_interface timer_interface_t;

#endif