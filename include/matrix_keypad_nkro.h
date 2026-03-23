#ifndef MATRIX_KEYPAD_NKRO_H
#define MATRIX_KEYPAD_NKRO_H


//#include "keypad_interface.h"
#include <stdint.h>
#include "stdlib.h"

#define     KPAD_MAX_ROWS       CONFIG_MATRIX_KEYPAD_MAX_ROWS
#define     KPAD_MAX_COLS       CONFIG_MATRIX_KEYPAD_MAX_COLS

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

const char *key_event_to_str(key_event_t event);

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







typedef struct {
    uint8_t         mode;
    uint8_t        *keymap;      // flat [total_rows * total_cols] — caller owns this
    uint8_t        *row_gpios;   // [total_rows]
    uint8_t         total_rows;
    uint8_t        *col_gpios;   // [total_cols]
    uint8_t         total_cols;
    uint8_t         max_simultaneous_keys;
    keypadCallback  cb;
} keypad_config_t;




int keypadCreate(keypad_config_t* config, keypad_interface_t** out_if);

#endif
