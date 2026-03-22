/*This is the Timer interface reqquired by the keypad*/

#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H
#include <stdint.h>



typedef enum timer_run_type{
    TIMER_ONESHOT,
    TIMER_PERIODIC
}timer_run_type_t;


typedef enum timer_event_id{
    TIMER_EVENT_STARTED,
    //TIMER_EVENT_ONESHOT_COMPLETE,
    TIMER_EVENT_ELAPSED,
    TIMER_EVENT_STOPPED
}timer_event_t;


/*
typedef struct timer_event{
    timer_event_id_t event_type;
}timer_event_t;

*/

typedef void (*timerCallback)(timer_event_t,void* creator_context,void* user_context);

struct timer_interface{
    int (*timerSetInterval)(struct timer_interface*,uint64_t interval);   
    int (*timerStart)(struct timer_interface*,timer_run_type_t type);   //Run once or periodic
    int (*timerStop)(struct timer_interface*);
    int (*timerRestart)(struct timer_interface*);
    uint64_t (*timerGetCurrentTime)();   // Static method, means class method and not instance method
    int (*timerRegisterCallback)(struct timer_interface*,timerCallback cb);
    int (*timerRegisterUserContext)(struct timer_interface*,void* user_context);
    //int (*timerRegisterCreaContext)(struct timer_interface*,void* user_context);
    int (*timerDestroy)(struct timer_interface*);
};

typedef struct timer_interface timer_interface_t;

#endif