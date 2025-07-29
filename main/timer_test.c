#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_timer.h"


void callback(timer_event_t* event){


    printf("\n event occured %d", event->event_type);
}

void app_main(void)
{


    my_timer_t timer;
    int count=0;
    timerCreate(&timer);

    timer.interface.timerRegisterCallback(&timer.interface,callback);

    timer.repeat_interval=2000000;
    timer.interface.timerRepeatStart(&timer.interface);

    while(1){

        vTaskDelay(pdMS_TO_TICKS(10000));
        count++;
        if(count==10)
            timer.interface.timerRepeatStop(&timer.interface);

    }

}
