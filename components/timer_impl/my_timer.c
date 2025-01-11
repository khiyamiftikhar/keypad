#include <stdio.h>
#include "esp_timer.h"
#include "esp_err.h"
#include "my_timer.h"


static void periodic_timer_callback(void* arg);
static void oneshot_timer_callback(void* arg);


#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



    



static int timerSetLongPressInterval(timer_interface_t* self,uint64_t long_press_interval){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    my_timer->long_press_interval=long_press_interval;
    return 0;

}


static int timerSetRepeatInterval(timer_interface_t* self,uint64_t repeat_interval){


    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    my_timer->repeat_interval=repeat_interval;
    return 0;

}



static int timerLongPressStart(timer_interface_t* self){
    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;


    
    return esp_timer_start_once((esp_timer_handle_t)my_timer->one_shot_timer, my_timer->long_press_interval);
    
}

static int timerLongPressStop(timer_interface_t* self){
    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    return esp_timer_stop((esp_timer_handle_t)my_timer->one_shot_timer);
}


//int (*timerLongPressReset)(struct timer_interface*);
static int timerRepeatStart(timer_interface_t* self){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    return esp_timer_start_periodic((esp_timer_handle_t)my_timer->periodic_timer, my_timer->repeat_interval);
    
}

static int timerRepeatStop(timer_interface_t *  self){
    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    return esp_timer_stop((esp_timer_handle_t)my_timer->periodic_timer);


}

static uint64_t timerGetCurrentTime(void){
    return esp_timer_get_time();
}



static int timerRegisterCallback(timer_interface_t* self,void(*callback)(timer_event_t*)){

    if(self==NULL || callback==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    my_timer->callback=callback;

    return 0;

}





/// @brief Create a timer. Assign all the members their respective values
/// @param self 
/// @return 
int timerCreate(my_timer_t* self){

    if(self==NULL)
        return -1;




     /* Create two timers:
     * 1. a periodic timer which will run every 0.5s, and print a message
     * 2. a one-shot timer which will fire after 5s, and re-start periodic
     *    timer with period of 1s.
     */

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .arg = (void*)self,
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    if(esp_timer_create(&periodic_timer_args, &periodic_timer)!=ESP_OK)
        return -1;
    /* The timer has been created but is not running yet */

    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &oneshot_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg = (void*)self,
            .name = "one-shot"
    };
    esp_timer_handle_t oneshot_timer;
    if(esp_timer_create(&oneshot_timer_args, &oneshot_timer)!=ESP_OK)
        return -1;



    self->one_shot_timer=(void*)oneshot_timer;
    self->periodic_timer=(void*)periodic_timer;
    self->interface.timerGetCurrentTime=timerGetCurrentTime;
    self->interface.timerLongPressStart=timerLongPressStart;
    self->interface.timerLongPressStop=timerLongPressStop;
    self->interface.timerRepeatStart=timerRepeatStart;
    self->interface.timerRepeatStop=timerRepeatStop;
    self->interface.timerRegisterCallback=timerRegisterCallback;
    /* Clean up and finish the example 
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(oneshot_timer));
        */
    return 0;
}



int timerDestroy(my_timer_t* self){

    if(self==NULL)
        return -1;

    esp_timer_handle_t periodic_timer=(esp_timer_handle_t)self->periodic_timer;
    esp_timer_handle_t one_shot_timer=(esp_timer_handle_t)self->one_shot_timer;
    

    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_stop(one_shot_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(one_shot_timer));
        
    return 0;





}



/// @brief Call back registered with the ESPIDF timer driver
/// @param arg 
static void periodic_timer_callback(void* arg){
    

    my_timer_t* my_timer=(my_timer_t*)arg;
    timer_event_t event;
    event.event_type=TIMER_REPEAT_EVENT;
    //Call the callback registered by the user
    my_timer->callback(&event);
    
}

/// @brief Call back registered with the ESPIDF timer driver
/// @param arg 
static void oneshot_timer_callback(void* arg)
{

    my_timer_t* my_timer=(my_timer_t*)arg;
    timer_event_t event;
    event.event_type=TIMER_LONG_PRESS_EVENT;
    //Call the callback registered by the user
    printf("\nalarm");
    my_timer->callback(&event);
}






