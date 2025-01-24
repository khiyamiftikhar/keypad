#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "my_timer.h"

static const char* TAG="timer impl";
static void timer_callback(void* arg);



#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



    



static int timerSetInterval(timer_interface_t* self,uint64_t interval){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    my_timer->interval=interval;
    return 0;

}





    
    
    










//int (*timerLongPressReset)(struct timer_interface*);
static int timerStart(timer_interface_t* self,timer_run_type_t run_type){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
 
        return -1;

    ESP_LOGI(TAG," interval is %llu",my_timer->interval);
    esp_err_t err;
    if(run_type==TIMER_PERIODIC)
        err=esp_timer_start_periodic((esp_timer_handle_t)my_timer->timer_handle, my_timer->interval);
    else{
        err=esp_timer_start_once((esp_timer_handle_t)my_timer->timer_handle, my_timer->interval);

    }
    
    return err;
}

static int timerStop(timer_interface_t *  self){
    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    return esp_timer_stop((esp_timer_handle_t)my_timer->timer_handle);


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
int timerCreate(my_timer_t* self, char* name,void (*callback)(timer_event_t*)){

    char timer_name[10];

    if(self==NULL || callback==NULL)
        return -1;




     /* Create two timers:
     * 1. a periodic timer which will run every 0.5s, and print a message
     * 2. a one-shot timer which will fire after 5s, and re-start periodic
     *    timer with period of 1s.
     */

    strncpy(timer_name,name,10);
    timer_name[9]='\0'; //strncpy does not null terminate if source string is >= n i.e 10 here   

    const esp_timer_create_args_t timer_args = {
            .callback = &timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .arg = (void*)self,
            .name = timer_name
    };

    esp_timer_handle_t timer_handle;
    if(esp_timer_create(&timer_args, &timer_handle)!=ESP_OK)
        return -1;

    /* The timer has been created but is not running yet */




    self->timer_handle=(void*)timer_handle;
    self->callback=callback;
    //self->interface.timerGetCurrentTime=timerGetCurrentTime;
    self->interface.timerStart=timerStart;
    self->interface.timerStop=timerStop;
    self->interface.timerSetInterval=timerSetInterval;
    self->interface.timerRegisterCallback=timerRegisterCallback;
    self->interface.timerGetCurrentTime=timerGetCurrentTime;
    /* Clean up and finish the example 
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(longpress_timer));
        */
    return 0;
}



int timerDestroy(my_timer_t* self){

    if(self==NULL)
        return -1;

    esp_timer_handle_t timer_handle=(esp_timer_handle_t)self->timer_handle;
    ESP_ERROR_CHECK(esp_timer_stop(timer_handle));
    return 0;

}



/// @brief Call back registered with the ESPIDF timer driver
/// @param arg 
static void timer_callback(void* arg){
    

    ESP_LOGI(TAG,"internal callback");

    my_timer_t* my_timer=(my_timer_t*)arg;
    timer_event_t event;

    //if(my_timer->type==TIMER_ONESHOT)
    event.event_type=TIMER_EVENT_ELAPSED;

    //Call the callback registered by the user
    my_timer->callback(&event);
    
}






