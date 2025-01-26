#include "unity.h"
#include "my_timer.h"
#include "esp_log.h"

static const char* TAG="test timer";

void timer_callback(timer_event_t event){

    ESP_LOGI(TAG,"event is %d",event->event_type);

}

 static my_timer_t timer;
 //static my_timer_t periodic_timer;

TEST_CASE("TIMER: Create Timer","[Unit Test: Timer]"){

   

   timerCreate(&timer,"random",&timer_callback);
   

   timer.interface.timerSetInterval(&timer.interface,2000000);
   
}






TEST_CASE("TIMER: Start One Shot","[Unit Test: Timer]"){

    
   timer.interface.timerStart(&(timer.interface),TIMER_ONESHOT);
   

}


TEST_CASE("TIMER: Start Periodic","[Unit Test: Timer]"){

    
   timer.interface.timerStart(&(timer.interface),TIMER_PERIODIC);
   

}


TEST_CASE("TIMER: Stop","[Unit Test: Timer]"){

    
   timer.interface.timerStop(&(timer.interface));
   

}


