#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "my_timer.h"
#include "keypad_button.h"


static const char* TAG="button";
#define TOTAL_BUTTON_OBJECTS        CONFIG_TOTAL_BUTTON_OBJECTS
#define DEBOUNCING_DURATION         (CONFIG_DEBOUNCE_DURATION*1000)   //Convert to micro
#define LONG_PRESS_DURATION         (CONFIG_LONG_PRESS_DURATION*1000)
#define REPEAT_PRESS_DURATION       (CONFIG_REPEAT_PRESS_DURATION*1000)





/*States used internally
Two consecutive detects will result is pressed debouncing and then timer will be allocated
Similarly two consecutive timer expires will result in going back to IDLE and thus deallocation
*/
typedef enum{
    BUTTON_STATE_IDLE=0,        //With the timer object deallocated 
    BUTTON_STATE_PROBABLE_PRESS,        //BOUNCE timeis set to 0
    BUTTON_STATE_PRESSED_BOUNCING_BREAK, //If timer expires in the BOUNCING_MAKE, it will come to this break
    BUTTON_STATE_PRESSED_BOUNCING_MAKE, //Timer allocated, if it stays BOUNCING, for BOUNCING_TIME
    BUTTON_STATE_PRESSED,   //Once this state is reached, timer expire will not make it go back, but forward
    BUTTON_STATE_PRESSED_LONG,
    BUTTON_STATE_RELEASED_BOUNCING_BREAK,   //Make break name is choosen for the scenerio where state moves to and fro between these two
    BUTTON_STATE_RELEASED_BOUNCING_MAKE,
    BUTTON_STATE_RELEASED,
     //BUTTON_STATE_PRESSED_REPEAT
    }button_state_t;


typedef struct keypad_button{
    uint8_t button_index;
    uint8_t button_id;
    timer_interface_t* timer;                //typecasted internally, This addded Later->bcz timer pool store void pointer    
    void* timer_pool;           //to allocate and deallocate the timer object
    uint32_t time_period;       //The time period for the timer.
    uint32_t previous_time;     //For keeping track of debouncing duration and long press
    button_state_t state;
    bool long_pressed;
    buttonCallBack cb;        
    void* context;              //To know which user (keyboard) instance it belongs to
    button_interface_t interface;
    uint32_t debug_previous_time;     //For keeping track of intervals between state machine runs
}keypad_button_t;


typedef struct button_pool{
    keypad_button_t button_array[TOTAL_BUTTON_OBJECTS];
    uint8_t count;

}button_pool_t;


static button_pool_t pool={0};

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))





#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})








static timer_interface_t* timerAllocate(pool_alloc_interface_t* timer_pool){
    timer_interface_t* timer=(timer_interface_t*)timer_pool->poolDrain(timer_pool);
    return timer;
}

static void timerReturn(pool_alloc_interface_t* timer_pool,timer_interface_t* timer){
    timer_pool->poolFill(timer_pool,(void*)timer);
    //Must set it to NULL
    timer=NULL;
}







static int buttonStateUpdateEventHandler(button_interface_t* self,button_state_update_event_t evt){

    keypad_button_t* btn = container_of(self,keypad_button_t,interface);
    button_state_t* state=&btn->state;
    button_state_t current_button_state=*state;
    button_state_t next_button_state=*state;

    
    pool_alloc_interface_t* timer_pool = (pool_alloc_interface_t*) btn->timer_pool;


    if(btn->timer==NULL){
        btn->timer=timerAllocate(timer_pool);
        
        //if unable to allocater timer
        if(btn->timer==NULL){
            ESP_LOGI(TAG,"not allocated");
            return 1;
        }
        
        ESP_LOGI(TAG,"allocated %d",btn->button_index);
        //Add the user context, to be used in the timer callback in the main keypad   
        btn->timer->timerRegisterUserContext(btn->timer,(void*)self);
        //This is the first occurance so set previous time to 0;
        btn->previous_time=0;   //For debouncing and long press
        btn->debug_previous_time=0;//To keep track of duration between state machine runs
    }

    timer_interface_t* timer=btn->timer;
    uint32_t time_period=btn->time_period;

    uint32_t* previous_time=&btn->previous_time;

    uint32_t current_time=timer->timerGetCurrentTime();
    //This data will be sent using callback whenever a significant event occurs
    button_event_data_t evt_data={.button_id=btn->button_id,
                                    .timestamp=current_time};
    
    evt_data.timestamp=current_time;

    //ESP_LOGI(TAG,"button mp event %d",evt);
    //ESP_LOGI(TAG,"pt %"PRIu32 " ct %"PRIu32 " diff %"PRIu32,btn->debug_previous_time,current_time,current_time-btn->debug_previous_time);

    switch(current_button_state){
        
        case BUTTON_STATE_IDLE:
                                if(evt==BUTTON_STATE_EVENT_PRESSED){
                                    next_button_state=BUTTON_STATE_PROBABLE_PRESS;
                                    ESP_LOGI(TAG,"t interval %"PRIu32,btn->time_period);
                                    timer->timerSetInterval(timer,btn->time_period);
                                    timer->timerStart(timer,TIMER_ONESHOT);
                                }
                                //Else can never come bcz timer not started
                                else{
                                    if(timer!=NULL){

                                        ESP_LOGI(TAG,"dealloc %d",btn->button_index);
                                        timer->timerStop(timer);
                                            //Return the timer to pool
                                        timerReturn(timer_pool,timer);
                                            //This is problematic. What if the same button is pressed again
                                            //before this NULL statement runs. must be throughly analyzed
                                        btn->timer=NULL;
                                    }
                                    next_button_state=current_button_state;
                                }
                                break;
                                

                              
        case BUTTON_STATE_PROBABLE_PRESS:
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        
                                        next_button_state=BUTTON_STATE_PRESSED_BOUNCING_BREAK;
                                        *previous_time=timer->timerGetCurrentTime();
                                    }

                                    else {//If timer elapsed event comes
                                        next_button_state=BUTTON_STATE_IDLE;
                                        timer->timerStop(timer);
                                            //Return the timer to pool
                                        timerReturn(timer_pool,timer);
                                            //This is problematic. What if the same button is pressed again
                                            //before this NULL statement runs. must be throughly analyzed
                                        btn->timer=NULL;
                                    }
                                    
                                    //Restart the timer irrespective of what event occurs
                                    timer->timerRestart(timer);

                                    break;

        case BUTTON_STATE_PRESSED_BOUNCING_BREAK:                       //If timer expires in the BOUNCING_MAKE, it will come to this break
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        next_button_state=BUTTON_STATE_PRESSED_BOUNCING_MAKE;
                                        *previous_time=timer->timerGetCurrentTime();
                                    }
                                    else{
                                        next_button_state=BUTTON_STATE_PROBABLE_PRESS;
                                    }
                                    //Restart the timer irrespective of what event occurs
                                    timer->timerRestart(timer);
                                    break;
                                    

        case BUTTON_STATE_PRESSED_BOUNCING_MAKE: //Timer allocated, if it stays BOUNCING, for BOUNCING_TIME
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        //ESP_LOGI(TAG,"pt %"PRIu32 " ct %"PRIu32 " diff %"PRIu32,current_time,*previous_time,current_time-*previous_time);
                                        if((current_time-*previous_time)>DEBOUNCING_DURATION){
                                             next_button_state=BUTTON_STATE_PRESSED;
                                             //Again record time for long press detection
                                            *previous_time=timer->timerGetCurrentTime();

                                            evt_data.event=BUTTON_EVENT_PRESSED;
                                            ESP_LOGI(TAG,"button index %d, val %d",btn->button_index,evt_data.button_id);
                                            btn->cb(btn->button_index,&evt_data,btn->context);
                                        }
                                        else
                                            next_button_state=current_button_state;
                                    }
                                    else{
                                        next_button_state=BUTTON_STATE_PRESSED_BOUNCING_BREAK;
                                    }

                                    //Restart the timer irrespective of what event occurs
                                    timer->timerRestart(timer);

                                    break;

        case BUTTON_STATE_PRESSED:
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        //ESP_LOGI(TAG,"pt %"PRIu32 " ct %"PRIu32 " diff %"PRIu32,current_time,*previous_time,current_time-*previous_time);
                                        if((current_time-*previous_time)>LONG_PRESS_DURATION){
                                            next_button_state=BUTTON_STATE_PRESSED_LONG;
                                            

                                            //Again record time for repeat press detection
                                            *previous_time=timer->timerGetCurrentTime();
                                            evt_data.event=BUTTON_EVENT_PRESSED_LONG;
                                            //So that comes back to this state frm BUTTON_STATE_RELEASED_BOUNCING_BREAK if it is true
                                            btn->long_pressed=true;
                                            btn->cb(btn->button_index,&evt_data,btn->context);
                                            
                                        }
                                        else{
                                            next_button_state=current_button_state;
                                        }
                                        

                                    }
                                    else{
                                        next_button_state=BUTTON_STATE_RELEASED_BOUNCING_BREAK;
                                    }

                                    //Restart the timer irrespective of what event occurs
                                    timer->timerRestart(timer);
                                    
                                    break;                                        

        case BUTTON_STATE_PRESSED_LONG:
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        //ESP_LOGI(TAG,"pt %"PRIu32 " ct %"PRIu32 " diff %"PRIu32,current_time,*previous_time,current_time-*previous_time);
                                        if((current_time-*previous_time)>REPEAT_PRESS_DURATION){
                                             //Again record time for repeat press detection
                                             //This info must be propagated to user
                                            *previous_time=timer->timerGetCurrentTime();
                                            evt_data.event=BUTTON_EVENT_PRESSED_REPEAT;
                                            btn->cb(btn->button_index,&evt_data,btn->context);
                                        }
                                        next_button_state=current_button_state;
                                    }
                                    else{
                                        next_button_state=BUTTON_STATE_RELEASED_BOUNCING_BREAK;
                                    }
                                    timer->timerRestart(timer);
                                    break;                                        
        case BUTTON_STATE_RELEASED_BOUNCING_BREAK:
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                        if(btn->long_pressed==true)
                                            next_button_state=BUTTON_STATE_PRESSED_LONG;
                                        else
                                            next_button_state=BUTTON_STATE_PRESSED;
                                    }
                                            
                                    else{
                                            next_button_state=BUTTON_STATE_RELEASED_BOUNCING_MAKE;
                                    }
                                    timer->timerRestart(timer);

                                    break;

        case BUTTON_STATE_RELEASED_BOUNCING_MAKE:
                                    if(evt==BUTTON_STATE_EVENT_PRESSED){
                                            next_button_state=BUTTON_STATE_RELEASED_BOUNCING_BREAK;
                                            timer->timerRestart(timer);
                                    }
                                            
                                    else{
                                            next_button_state=BUTTON_STATE_IDLE;
                                            btn->long_pressed=false;
                                            ESP_LOGI(TAG,"dealloc %d",btn->button_index);
                                            //Stop the timer
                                            timer->timerStop(timer);
                                            //Return the timer to pool
                                            timerReturn(timer_pool,timer);
                                            //This is problematic. What if the same button is pressed again
                                            //before this NULL statement runs. must be throughly analyzed
                                            btn->timer=NULL;
                                            evt_data.event=BUTTON_EVENT_RELEASED;
                                            btn->cb(btn->button_index,&evt_data,btn->context);
                                    }
                                    
                                    break;
                                    
        case BUTTON_STATE_RELEASED:
                                //same as IDLE
                                if(evt==BUTTON_STATE_EVENT_PRESSED)
                                    next_button_state=BUTTON_STATE_PROBABLE_PRESS;
                                else
                                    next_button_state=current_button_state;
                                break;
                                
    }

    *state=next_button_state;
    btn->debug_previous_time=current_time;
    ESP_LOGI(TAG,"bt index %d st %d",btn->button_index,*state);
    return 0;
}





/// @brief Get one object from static array. not thread safe
/// @return 
static keypad_button_t* poolGet(){
    
    if(pool.count==TOTAL_BUTTON_OBJECTS)
        return NULL;
    
    keypad_button_t* self=&pool.button_array[pool.count];
        pool.count++;
    return self;

}

static void poolReturn(){

    pool.count--;

}




button_interface_t* keypadButtonCreate(button_config_t* config){
    if(config==NULL)
        return NULL;

    //Get button object from the static button pool
    keypad_button_t* self=poolGet();

    if(self==NULL)
        return NULL;
    if(config->timer_pool==NULL){
        ESP_LOGI(TAG,"no pool");
        return NULL;
    }
    //Specifically did this for the bool long_pressed
    memset(self,0,sizeof(keypad_button_t));        
    uint8_t button_index=config->button_index;
    uint8_t button_id=config->button_id;            //e.g 'A' etc
    //void* timer_pool=;           //to allocate and deallocate the timer object. Timer is allocated dynamicalyy but atomically nonblocking way the total avialble is checked first to decide whether wait ior not
    uint32_t scan_time_period=config->scan_time_period;       //The time period for the timer. The timer elapses after this time,
                                //and timer handler calls the callback, which is used to update state
    buttonCallBack cb=config->cb;

    self->button_id=button_id;
    self->button_index=button_index;
    self->state=BUTTON_STATE_IDLE;
    self->time_period=scan_time_period;                   //in microseconds , must be greater than prober time  period

    
    self->timer_pool=config->timer_pool;
    self->timer=NULL;                               //Timer is allocated dynamically
    //self->timer_pool=timer_pool;
    self->context=config->context;
    self->cb=cb;
    self->interface.buttonEventInform=buttonStateUpdateEventHandler;
    return &self->interface;
}
