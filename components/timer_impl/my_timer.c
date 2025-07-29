#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "my_timer.h"

#define     MAX_OBJECTS         CONFIG_MAX_OBJECTS

static const char* TAG="timer impl";
static void timer_callback(void* arg);



#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})




typedef struct my_timer{
    uint64_t interval;         //Time after which alarm will trigger; private but set using interface
    timer_run_type_t type;      //used when restarting, private data internally managed
    esp_timer_handle_t timer_handle;               //timer object of espidf, private data internally managed
    timerCallback callback;
    timer_interface_t interface;
    void* user_context;      //So that user can be identified, as which user's (button's) time is elapsed
    void* creator_context;  //So that the creator can be identified. For example if used in the keypad code, then to which keypad it belogs to
}my_timer_t;


//Dont confuse this with the pool alloc component which is for dynamic alloc/free
//This is basic structure for statically allocating object

typedef struct timer_pool{

    my_timer_t timer_list[MAX_OBJECTS];
    uint8_t count;
}timer_pool_t;


static timer_pool_t timers={0};

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

    //ESP_LOGI(TAG," interval is %"PRIu64,my_timer->interval);
    esp_err_t err;
    my_timer->type=run_type;
    if(run_type==TIMER_PERIODIC)
        err=esp_timer_start_periodic(my_timer->timer_handle, my_timer->interval);
    else{
        err=esp_timer_start_once(my_timer->timer_handle, my_timer->interval);

    }
    
    return err;
}




static int timerStop(timer_interface_t *  self){
    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
        return -1;

    return esp_timer_stop(my_timer->timer_handle);


}

static int timerRestart(timer_interface_t* self){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    if(my_timer==NULL)
         return -1;
    timer_run_type_t run_type=my_timer->type;

    //ESP_LOGI(TAG," interval is %llu",my_timer->interval);
    esp_err_t err;

    esp_timer_stop(my_timer->timer_handle);

    if(run_type==TIMER_PERIODIC)
        err=esp_timer_start_periodic(my_timer->timer_handle, my_timer->interval);
    else{
        err=esp_timer_start_once(my_timer->timer_handle, my_timer->interval);

    }
    
    return err;
}


static uint64_t timerGetCurrentTime(void){
    return esp_timer_get_time();
}



static int timerRegisterCallback(timer_interface_t* self,timerCallback cb){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    my_timer->callback=cb;

    return 0;

}

static int timerRegisterContext(timer_interface_t* self,void* user_context){

    if(self==NULL)
        return -1;

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    my_timer->user_context=user_context;

    return 0;

}


int timerDestroy(timer_interface_t* self){

    if(self==NULL)
        return -1;
    

    my_timer_t* my_timer=container_of(self,my_timer_t,interface);

    esp_timer_handle_t timer_handle=my_timer->timer_handle;
    ESP_ERROR_CHECK(esp_timer_stop(timer_handle));
    return 0;

}



/// @brief Get one element of pool, and increment the count. Not thread safe
/// @return 
static my_timer_t* poolGet(){
    
    if(timers.count==MAX_OBJECTS)
        return NULL;

    my_timer_t* self=&timers.timer_list[timers.count];
        timers.count++;
    return self;
}

static void poolReturn(){

    timers.count--;

}





/// @brief Create a timer. Assign all the members their respective values
/// @param self 
/// @param user_context This timer returns timer_interface pointer which could be pointer member of a struct which can have multiple instance. So user_context to figure out which instance
/// @return 
timer_interface_t* timerCreate(char* name,timerCallback cb,void* creator_context){

    char timer_name[10];
    my_timer_t* self=poolGet();

    
    
    if(self==NULL || cb==NULL)
        return NULL;




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
        return NULL;

    /* The timer has been created but is not running yet */




    self->timer_handle=timer_handle;
    self->callback=cb;
    self->creator_context=creator_context;
    //self->interface.timerGetCurrentTime=timerGetCurrentTime;
    self->interface.timerStart=timerStart;
    self->interface.timerStop=timerStop;
    self->interface.timerRestart=timerRestart;
    self->interface.timerSetInterval=timerSetInterval;
    self->interface.timerRegisterCallback=timerRegisterCallback;
    self->interface.timerGetCurrentTime=timerGetCurrentTime;
    self->interface.timerDestroy=timerDestroy;
    self->interface.timerRegisterUserContext=timerRegisterContext;    
    /* Clean up and finish the example 
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(longpress_timer));
        */
    return &self->interface;
}






/// @brief Call back registered with the ESPIDF timer driver
/// @param arg 
static void timer_callback(void* arg){
    

    //ESP_LOGI(TAG,"internal callback");

    my_timer_t* my_timer=(my_timer_t*)arg;
    timer_event_t event;

    //if(my_timer->type==TIMER_ONESHOT)
    event=TIMER_EVENT_ELAPSED;

    //Call the callback registered by the user
    my_timer->callback(event,my_timer->creator_context,my_timer->user_context);
    
}






