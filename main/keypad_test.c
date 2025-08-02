#include <stdio.h>
#include <esp_log.h>
#include "keypad_dev.h"



static const char* TAG="main";

void keyPadHandler(key_event_t event,keypad_event_data_t* evt_data){

        ESP_LOGI(TAG, "event %d   key_val %s", event,(char*)&evt_data->key_id);


}


void app_main(){


    esp_log_level_set("button", ESP_LOG_NONE);
    esp_log_level_set("keypad", ESP_LOG_NONE);
    
    // Keep your app logs visible
    //esp_log_level_set("ketboard_impl", ESP_LOG_NONE);

    uint8_t col_gpios[]={12,13,14,15};
    uint8_t row_gpios[]={18,19,22,23};
    keypad_config_t config={    .cb=keyPadHandler,
                                .col_gpios=col_gpios,
                                .row_gpios=row_gpios,
                                .total_cols=4,
                                .total_rows=4,
                                .keymap={{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}}
                            };

        

    keypad_interface_t* keypad = keypadCreate(&config);

    
}