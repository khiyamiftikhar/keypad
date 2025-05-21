#include "unity.h"
#include "my_timer.h"
#include "esp_log.h"

static const char* TAG="test timer";

void timer_callback(timer_event_t event){

    ESP_LOGI(TAG,"event is %d",event);

}

 static timer_interface_t* timer;
 //static my_timer_t periodic_timer;

 /*
void("TIMER: Create Timer","[Unit Test: Timer]"){

   

   timer=timerCreate("random",&timer_callback);
   

   timer->timerSetInterval(timer,2000000);
   
}
*/


void setUp(){
   timer=timerCreate("random",&timer_callback);
   timer->timerSetInterval(timer,2000000);


}




TEST_CASE("TIMER: Start One Shot","[Unit Test: Timer]"){

    
   timer->timerStart(timer,TIMER_ONESHOT);
   

}


TEST_CASE("TIMER: Start Periodic","[Unit Test: Timer]"){

    
   timer->timerStart(timer,TIMER_PERIODIC);
   

}


TEST_CASE("TIMER: Stop","[Unit Test: Timer]"){

    
   timer->timerStop(timer);
   

}


