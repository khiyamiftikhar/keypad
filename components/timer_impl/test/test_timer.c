#include "unity.h"
#include "my_timer.h"



void timer_callback(timer_event_t* event){

    printf("event is %d",event->event_type);

}

 static my_timer_t timer;

TEST_CASE("TIMER: Create","[Unit Test: Timer]"){

   

   timerCreate(&timer);
   timer.callback=timer_callback;

   timer.long_press_interval=1000000;
   timer.repeat_interval=2000000;
   

}


TEST_CASE("TIMER: Start One Shot","[Unit Test: Timer]"){

    
   timer.interface.timerLongPressStart(&(timer.interface));
   

}


TEST_CASE("TIMER: Start Periodic","[Unit Test: Timer]"){

    
   timer.interface.timerRepeatStart(&(timer.interface));
   

}
