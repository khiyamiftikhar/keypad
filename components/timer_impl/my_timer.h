
#ifndef MY_TIMER_H
#define MY_TIMER_H

#include "timer_interface.h"

#include "stdint.h"




typedef struct my_timer{
    uint64_t interval;         //Time after which alarm will trigger;
    
    void* timer_handle;               //timer object of espidf
    void (*callback)(timer_event_t*);
    timer_interface_t interface;
    
}my_timer_t;


int timerCreate(my_timer_t* self,char* name, void (*callback)(timer_event_t*));


int timerDestroy(my_timer_t* self);


#endif