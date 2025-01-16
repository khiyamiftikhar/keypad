#include "unity.h"
#include "my_timer.h"
#include "esp_log.h"

static const char* TAG="test timer";

void timer_callback(timer_event_t* event){

    ESP_LOGI(TAG,"event is %d",event->event_type);

}

 static my_timer_t oneshot_timer;
 static my_timer_t periodic_timer;

TEST_CASE("TIMER: Create One shot","[Unit Test: Timer]"){

   

   timerCreate(&oneshot_timer,TIMER_ONESHOT,&timer_callback);
   

   oneshot_timer.interface.timerSetInterval(&oneshot_timer.interface,10000000);
   
}

TEST_CASE("TIMER: Create Periodic","[Unit Test: Timer]"){

   

timerCreate(&periodic_timer,TIMER_PERIODIC,&timer_callback);
   

periodic_timer.interface.timerSetInterval(&periodic_timer.interface,5000000);
   

}





TEST_CASE("TIMER: Start One Shot","[Unit Test: Timer]"){

    
   oneshot_timer.interface.timerStart(&(oneshot_timer.interface));
   

}


TEST_CASE("TIMER: Start Periodic","[Unit Test: Timer]"){

    
   periodic_timer.interface.timerStart(&(periodic_timer.interface));
   

}


TEST_CASE("TIMER: Stop One Shot","[Unit Test: Timer]"){

    
   oneshot_timer.interface.timerStop(&(oneshot_timer.interface));
   

}


TEST_CASE("TIMER: Stop Periodic","[Unit Test: Timer]"){

    
   periodic_timer.interface.timerStop(&(periodic_timer.interface));
   

}

