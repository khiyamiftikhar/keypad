
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unity.h"
#include "pwm_line.h"
#include "esp_log.h"
static const char* TAG="test PWM";

pwm_line_t line1;
pwm_line_t line2;

void setUp() {
    ESP_LOGI(TAG, "Setting up test environment...");
    pwm_config_t config1={.frequency=100,
                        .duty_cycle=25,
                        .phase=0,
                        .phase_cut_on_duty=20,
                        .gpio=5,
                        .channel_number=0,
                        };

    pwm_config_t config2={.frequency=100,
                        .duty_cycle=25,
                        .phase=90,
                        .phase_cut_on_duty=20,
                        .gpio=18,
                        .channel_number=1,
                        };
    pwmCreate(&line1,&config1);
    pwmCreate(&line2,&config2);
}






TEST_CASE("PWM: Stop and Start","[Unit Test: PWM]"){

    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG,"Stopping");
    line1.interface.pwmStop(&line1.interface);
    line2.interface.pwmStop(&line2.interface);
    
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG,"REsuming");
    line1.interface.pwmStart(&line1.interface);
    line2.interface.pwmStart(&line2.interface);

   

}



void tearDown(){



}