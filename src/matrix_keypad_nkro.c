
#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include "matrix_keypad_nkro.h"
#include "my_timer.h"
#include "pool_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "scan_manager.h"
#include "interleaved_pwm.h"
//#include "pulse_decoder.h"
#include "keypad_button.h"
#include "init.h"


#define MAX_KEYPADS              CONFIG_MATRIX_KEYPAD_MAX_KEYPADS
#define MAX_SIMULTANEOUS_KEYS    CONFIG_MATRIX_KEYPAD_MAX_SIMULTANEOUS_KEYS
#define MAX_BUTTONS             (CONFIG_MATRIX_KEYPAD_MAX_COLS*CONFIG_MATRIX_KEYPAD_MAX_ROWS)

//These #defines are internal
//This first queue is the one that gets both scanner and timer events and passes to the corresponding button
#define MAX_INTERNAL_EVENT_QUEUE_ELEMENTS               100
//This queue gets the user evebts   
#define MAX_USER_EVENT_QUEUE_ELEMENTS                   50

//#define QUEUE_LENGTH                                100

static const char* TAG="keypad";

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))



#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


//Events for the MP queue
typedef enum{
    MP_EVENT_BUTTON_PRESS=0,
    MP_EVENT_TIMER_ELAPSED
}mp_event_t;



typedef struct{
    mp_event_t event;
    button_interface_t* button;          //Which button and which timer is causing this event
}mp_event_data_t;


/*typedef struct{
    key_event_t event;
    uint8_t button_id;           //Which button and which timer is causing this event
    uint32_t time_stamp;
}key_event_data_t;*/

//For The MP. Both the timer events and scan events will queue here
typedef struct queue_mp_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(mp_event_data_t)*MAX_INTERNAL_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_mp_event_handle_t;


typedef struct queue_user_event_handle{
    QueueHandle_t handle;
    StaticQueue_t queue_meta_data;
    uint8_t buff[sizeof(keypad_event_data_t)*MAX_USER_EVENT_QUEUE_ELEMENTS];      //This must be equal to total void pointers, sizeof(void*)*total_elements
    //size_t object_size;         //Not required as it is a void pointer
}queue_user_event_handle_t;





typedef struct keypad_dev {
    /* --- dimensions (set once at creation) --- */
    uint8_t total_rows;
    uint8_t total_cols;
    uint8_t max_simultaneous_keys;
    uint8_t max_buttons;

    /* --- dynamic arrays (pointers into the single alloc block) --- */
    uint8_t          *keymap;      // flat [total_rows * total_cols]
    uint8_t          *row_gpios;   // [total_rows]
    uint8_t          *col_gpios;   // [total_cols]
    scanner_interface_t  *scanner;     // it will internally manged col_no decoders
    interleaved_pwm_interface_t* prober;    //interleaved_pwm
    timer_interface_t **timers;    // [max_simultaneous_keys]  — ptr array
    button_interface_t **button;   // [max_buttons]            — ptr array
    uint32_t* pwm_width;        //[total rows]

    /* --- fixed-size members (unchanged) --- */
    key_press_mode_t         mode;
    keypad_interface_t       interface;
    pool_alloc_interface_t  *timer_pool;
    queue_mp_event_handle_t  mp_event_queue;
    queue_user_event_handle_t user_event_queue;
    TaskHandle_t             mp_queue_task;
    TaskHandle_t             user_queue_task;
    keypadCallback           cb;
} keypad_dev_t;


static void scannerEventHandler(scanner_event_data_t* event_data,void* context){

    //Get the keypad_dev_t instance
    keypad_dev_t* self=(keypad_dev_t*)context;

    if(self->mp_event_queue.handle==NULL)
        return;

    uint8_t row=event_data->source_number;
    uint8_t col=event_data->line_number;
    uint8_t total_cols=self->total_cols;
    uint8_t button_index=((row*total_cols)+col);
    ESP_LOGI(TAG,"row %d, col %d, button index %d",row,col,button_index);
    button_interface_t* button=self->button[button_index];

    mp_event_data_t mp_event_data;
    mp_event_data.event=MP_EVENT_BUTTON_PRESS;
    mp_event_data.button=button;
    //ESP_LOGI(TAG,"before mp  sending");
    xQueueSend(self->mp_event_queue.handle,&mp_event_data,portMAX_DELAY);
    //ESP_LOGI(TAG,"after mp  sending");

    

}


static void timerEventHandler(timer_event_t event,void* creator_context,void* user_context){
    
    //keypad_button_t* self=(keypad_dev_t*) context;
    ESP_LOGI(TAG,"t hand");
    keypad_dev_t* keypad=(keypad_dev_t*) creator_context;
    mp_event_data_t event_data;
    button_interface_t* button=(button_interface_t*)user_context;
    event_data.button=button;
    event_data.event=MP_EVENT_TIMER_ELAPSED;         //Although different enum types , but ennum value is same i.e 1 for timer elapse

    QueueHandle_t queue=keypad->mp_event_queue.handle;
    //ESP_LOGI(TAG,"timer over");
    //Push the timer elapsed event with the context of the user (button) for which the timer elapsed.
    xQueueSend(queue,&event_data,portMAX_DELAY);

}




static void task_mp_queue(void* args){

    button_interface_t* button;
    keypad_dev_t* self=(keypad_dev_t*)args;
    QueueHandle_t queue_handle=(QueueHandle_t) self->mp_event_queue.handle;
    mp_event_data_t mp_event_data={0};
    
    while(1){
        if(xQueueReceive(queue_handle,&mp_event_data,portMAX_DELAY)==pdTRUE){

            //ESP_LOGI(TAG,"problem here %d",mp_event_data.event);
            button=mp_event_data.button;
            if(button==NULL || button->buttonEventInform==NULL)
                continue;
            //ESP_LOGI(TAG,"not so %d",mp_event_data.event);
            switch(mp_event_data.event){
                case MP_EVENT_BUTTON_PRESS:    button->buttonEventInform(button,BUTTON_STATE_EVENT_PRESSED); break;
                
                case MP_EVENT_TIMER_ELAPSED:    button->buttonEventInform(button,BUTTON_STATE_EVENT_TIMER_ELAPSED); break;

                //This is not useful or well though out. just written for the sake of writing a default statement
                default:    button->buttonEventInform(button,2); break;

            }
            
        }

    }

}

static void task_user_queue(void* args){
    keypad_dev_t* self=(keypad_dev_t*)args;
    QueueHandle_t queue_handle=(QueueHandle_t) self->user_event_queue.handle;
    keypad_event_data_t key_event_data={0};


    while(1){


        if(xQueueReceive(queue_handle,&key_event_data,portMAX_DELAY)==pdTRUE){
            ESP_LOGI(TAG,"sending key id %d",key_event_data.key_id);
            self->cb(key_event_data.event,&key_event_data);
            
        }


    }



}



static void buttonEventHandler(uint8_t button_index,button_event_data_t* evt,void* context){

    keypad_dev_t* self=(keypad_dev_t*) context;

    QueueHandle_t queue=self->user_event_queue.handle;

    keypad_event_data_t keypad_event_data={0};

    ESP_LOGI(TAG,"id %d, val %d",button_index,evt->button_id);
    keypad_event_data.event=evt->event;
    keypad_event_data.key_id=evt->button_id;    
    keypad_event_data.time_stamp=evt->timestamp;

    xQueueSend(queue,&keypad_event_data,portMAX_DELAY);
    
}


#define ALIGN_UP(sz, a)  (((sz) + (a) - 1) & ~((a) - 1))
keypad_dev_t* keypadAlloc(const keypad_config_t *config)
{
    /* --- validate --- */
    if (!config) return NULL;
    if (!config->total_rows || !config->total_cols) return NULL;
    if (!config->keymap) return NULL;
    if (!config->row_gpios || !config->col_gpios) return NULL;

    uint8_t rows         = config->total_rows;
    uint8_t cols         = config->total_cols;
    uint8_t max_sim_keys = config->max_simultaneous_keys;
    uint8_t total_buttons = rows * cols;

    size_t off = 0;

    /* --- struct (keep at start so free(dev) works) --- */
    size_t off_dev = 0;
    off += sizeof(keypad_dev_t);
    /* --- raw byte arrays --- */
    size_t off_keymap  = off; off += rows * cols * sizeof(uint8_t);
    size_t off_rowgpio = off; off += rows * sizeof(uint8_t);
    size_t off_colgpio = off; off += cols * sizeof(uint8_t);

    /* --- pulse widths --- */
    off = ALIGN_UP(off, _Alignof(uint32_t));
    size_t off_pulse_widths = off;
    off += rows * sizeof(uint32_t);

    /* --- timers (array of pointers) --- */
    off = ALIGN_UP(off, _Alignof(void *));
    size_t off_timers = off;
    off += max_sim_keys * sizeof(timer_interface_t *);

    /* --- buttons (array of pointers) --- */
    off = ALIGN_UP(off, _Alignof(void *));
    size_t off_buttons = off;
    off += total_buttons * sizeof(button_interface_t *);

    size_t total = off;

    /* --- allocate --- */
    uint8_t *block = calloc(1, total);
    if (!block) return NULL;

    keypad_dev_t *dev = (keypad_dev_t *)(block + off_dev);

    /* --- wire pointers --- */
    dev->keymap    = (uint8_t *)(block + off_keymap);
    dev->row_gpios = (uint8_t *)(block + off_rowgpio);
    dev->col_gpios = (uint8_t *)(block + off_colgpio);
    dev->pwm_width = (uint32_t *)(block + off_pulse_widths);
    dev->timers    = (timer_interface_t **)(block + off_timers);
    dev->button    = (button_interface_t **)(block + off_buttons);

    /* scanner and prober are interface handles assigned later */
    dev->scanner = NULL;
    dev->prober  = NULL;

    /* --- assign metadata --- */
    dev->total_rows            = rows;
    dev->total_cols            = cols;
    dev->max_simultaneous_keys = max_sim_keys;
    dev->max_buttons           = total_buttons;
    dev->mode = config->mode;
    dev->cb   = config->cb;

    
    /* --- copy input arrays --- */
    memcpy(dev->keymap,    config->keymap,    rows * cols);
    memcpy(dev->row_gpios, config->row_gpios, rows);
    memcpy(dev->col_gpios, config->col_gpios, cols);


    ESP_LOGI(TAG,"total inputs aloc %d",cols);

    ESP_LOGI(TAG,"input gpio 0 %d",dev->col_gpios[0]);

    return dev;
}


int keypadCreate(keypad_config_t* config, keypad_interface_t** out_if){

    uint8_t max_simultaneous_keys=(config->max_simultaneous_keys);
    uint8_t total_buttons=((config->total_cols)*(config->total_rows));

    if(config==NULL || 
       (max_simultaneous_keys>total_buttons)
        )
        return ESP_ERR_INVALID_ARG;

        
    ESP_LOGI(TAG,"total inputs before alloc %d",config->total_cols);
    
    keypad_dev_t* self=keypadAlloc(config);
    
    
    if(self==NULL)
        return ESP_ERR_NO_MEM;
    
    
    
    
    //It will config as well as fill the self->pwm_width because it calculates it
    esp_err_t ret=configKeypadOutput(&self->prober,self->row_gpios,self->pwm_width,self->total_rows);
    ESP_LOGI(TAG,"prober %d",ret);
    
    ESP_LOGI(TAG,"pw  %lu, %lu, %lu, %lu",self->pwm_width[0],self->pwm_width[1],self->pwm_width[2],self->pwm_width[3]);

    ret=configKeypadInput(&self->scanner,self->col_gpios,self->total_cols,self->total_rows,self->pwm_width,scannerEventHandler,(void*)self);
    ESP_LOGI(TAG,"scanner %d",ret);
    
    
    ret=configKeypadTimers(self->timers,config->max_simultaneous_keys,(void*)self,timerEventHandler);
    ESP_LOGI(TAG,"timer %d",ret);
    ret=configTimerPool(&self->timer_pool,self->timers,config->max_simultaneous_keys);
    ESP_LOGI(TAG,"timer pool %d",ret);
    ret=configKeypadButtons(self->button,self->keymap,total_buttons,(void*)self,self->prober->getTimePeriod(),buttonEventHandler,(void*)self);
    ESP_LOGI(TAG,"button %d",ret);

    self->mp_event_queue.handle=xQueueCreateStatic(MAX_INTERNAL_EVENT_QUEUE_ELEMENTS,sizeof(mp_event_data_t),self->mp_event_queue.buff,&self->mp_event_queue.queue_meta_data);

    self->user_event_queue.handle=xQueueCreateStatic(MAX_USER_EVENT_QUEUE_ELEMENTS,sizeof(keypad_event_data_t),self->user_event_queue.buff,&self->user_event_queue.queue_meta_data);

    ESP_LOGI(TAG, "Free heap before: %" PRIu32, esp_get_free_heap_size());
    xTaskCreate(task_mp_queue,"MP Queue Task",8192,(void*)self,5,&self->mp_queue_task);
    xTaskCreate(task_user_queue,"User Queue Task",4096,(void*)self,5,&self->user_queue_task);
    //Start scannning
    self->scanner->startScanning(self->scanner);
    //start probing
    self->prober->start(self->prober);
    
    *out_if=&self->interface;
    return ESP_OK;
        
}

/* ─────────────────────────────────────────────────────────────────────────────
 * ADD THIS to matrix_keypad_nkro.h  (public API section)
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Convert a key_event_t value to a human-readable C string.
 *
 * The returned pointer is to a string literal – never free it.
 *
 * @param  event   Any value of key_event_t
 * @return         "KEY_PRESSED" | "KEY_RELEASED" | "KEY_LONG_PRESSED" |
 *                 "KEY_REPEATED" | "UNKNOWN_EVENT"
 */
const char *key_event_to_str(key_event_t event);


/* ─────────────────────────────────────────────────────────────────────────────
 * ADD THIS to matrix_keypad_nkro.c  (implementation)
 * ───────────────────────────────────────────────────────────────────────────── */

const char *key_event_to_str(key_event_t event)
{
    switch (event) {
        case KEY_PRESSED:      return "KEY_PRESSED";
        case KEY_RELEASED:     return "KEY_RELEASED";
        case KEY_LONG_PRESSED: return "KEY_LONG_PRESSED";
        case KEY_REPEATED:     return "KEY_REPEATED";
        default:               return "UNKNOWN_EVENT";
    }
}

                                