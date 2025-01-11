
#ifndef MY_TIMER_H
#define MY_TIMER_H

#include "timer_interface.h"

#include "stdint.h"




typedef struct my_timer{
    uint64_t long_press_interval;                //Time after which alarm will trigger;
    uint64_t repeat_interval;                //Time after which alarm will trigger;
    void* one_shot_timer;               //Run only once, For long press;
    void* periodic_timer;               //Periodically for repeat mode;
    void (*callback)(timer_event_t*);
    timer_interface_t interface;
    
}my_timer_t;


int timerCreate(my_timer_t* self);

int timerDestroy(my_timer_t* self);


#endif