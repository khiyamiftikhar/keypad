#ifndef KEYPAD_DEV_H
#define KEYPAD_DEV_H


//#include "keypad_interface.h"
#include <stdint.h>
#include "stdlib.h"

#define     KPAD_MAX_ROWS       CONFIG_KPAD_MAX_ROWS
#define     KPAD_MAX_COLS       CONFIG_KPAD_MAX_COLS

typedef enum 
    {SINGLE,     //No repeat when kept press
     REPEAT     //Kept press after long press is considered a new entry
    }key_press_mode_t;

typedef enum 
    {KEY_PRESSED,
    KEY_RELEASED,
    KEY_LONG_PRESSED,
    KEY_REPEATED,
    }key_event_t;

typedef struct{
    key_event_t event;
    uint8_t key_id;
    uint32_t time_stamp;
}keypad_event_data_t;


typedef void (*keypadCallback)(key_event_t event,keypad_event_data_t* evt_data);

typedef struct keypad_interface{
    /*These attribures such as dimensions keymap etc are set during creation so not available
    int (*setDimensions)(uint8_t total_rows,uint8_t total_cols);
    int (*setKeyMap)(struct keypad_interface* self, uint8_t** map);
    int (*setMode)(struct keypad_interface* self,key_press_mode_t mode);
    int (*registerCallback)(struct keypad_interface* self,void (*callback)(key_event_t event));

    */
    //
    int (*getPressedKeyList)(struct keypad_interface* self,char* pressed_key_list,size_t size);

}keypad_interface_t;







typedef struct{
    uint8_t mode;
    uint8_t keymap[KPAD_MAX_ROWS][KPAD_MAX_COLS];
    uint8_t* row_gpios;
    uint8_t total_rows;
    uint8_t* col_gpios;
    uint8_t total_cols;
    keypadCallback cb;
    
}keypad_config_t;




keypad_interface_t* keypadCreate(keypad_config_t* config);

#endif
