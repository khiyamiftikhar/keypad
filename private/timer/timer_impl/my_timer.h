
#ifndef MY_TIMER_H
#define MY_TIMER_H

#include "timer_interface.h"

#include "stdint.h"






timer_interface_t* timerCreate(char* name,timerCallback cb,void* creator_context);


#endif