#include <stdio.h>
#include <esp_log.h>
#include "matrix_keypad_nkro.h"


static const char* TAG="main";

 
void keyPadHandler(key_event_t event, keypad_event_data_t *evt_data)
{
    
    ESP_LOGI(TAG, "event=%-18s  key='%c'  timestamp=%lums",
             key_event_to_str(event),
             (char)evt_data->key_id,
             (unsigned long)evt_data->time_stamp);
             
}
 

void app_main(){


    //esp_log_level_set("button", ESP_LOG_NONE);
    //esp_log_level_set("keypad", ESP_LOG_NONE);
    
    // Keep your app logs visible
    //esp_log_level_set("ketboard_impl", ESP_LOG_NONE);

    //Create an array of Column GPIOs
    uint8_t row_gpios[]={32,33,25,26};
    //Create an array of row GPIOs
    uint8_t col_gpios[]={27,14,13,15};

    //Create and fill config struct
keypad_config_t config = {
    .auto_repeat_disable=true,  //Will not generate repeat events (when button is kept pressed beyond long press)
    .max_simultaneous_keys=5,
    .cb         = keyPadHandler,
    .col_gpios  = col_gpios,
    .row_gpios  = row_gpios,
    .total_cols = 4,
    .total_rows = 4,
    .long_press_duration_us=2000000,      //2000000 us so 2s
    .repeat_press_duration_us=3000000,      // 1 second
    .keymap     = (uint8_t[]){
                    '1','2','3','A',
                    '4','5','6','B',
                    '7','8','9','C',
                    '*','0','#','D'
                  }
};
        
    //Instantiate a keypad Object
    keypad_interface_t* keypad;
    int ret = keypadCreate(&config,&keypad);
   
    
}