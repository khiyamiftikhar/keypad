#include "unity.h"
#include "my_gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
static const char* TAG="test gpio";

static my_gpio_t debug_gpio;
void gpio_callback(gpio_event_t event){

   
    uint8_t val=debug_gpio.interface.digitalRead(&debug_gpio.interface);
   esp_rom_printf("ISR: Read value = %d next %d\n", val,val^0x01);
    debug_gpio.interface.digitalWrite(&debug_gpio.interface,val^0x01);

}

 static my_gpio_t my_gpio;
 

TEST_CASE("GPIO: Create gpio","[Unit Test: GPIO]"){

   gpioCreate(&my_gpio,10);
   gpioCreate(&debug_gpio,8);
     
}

TEST_CASE("GPIO: Set Mode input","[Unit Test: GPIO]"){

   my_gpio.interface.gpioMode(&my_gpio.interface,INPUT_PULLUP);
   

}

TEST_CASE("GPIO: Set Mode output (for LED)","[Unit Test: GPIO]"){

   debug_gpio.interface.gpioMode(&debug_gpio.interface,INPUT_OUTPUT);

}




TEST_CASE("GPIO: Read debug Pin","[Unit Test: GPIO]"){


   ESP_LOGI(TAG,"hello");

    int val=debug_gpio.interface.digitalRead(&debug_gpio.interface);

   esp_rom_printf("LED pin: Read value = %d \n", val);
    //debug_gpio.interface.digitalWrite(&debug_gpio.interface,val^0x01);

}



TEST_CASE("GPIO: LED on","[Unit Test: GPIO]"){

   debug_gpio.interface.digitalWrite(&debug_gpio.interface,1);
}

TEST_CASE("GPIO: LED off","[Unit Test: GPIO]"){

   debug_gpio.interface.digitalWrite(&debug_gpio.interface,0);
   

}




TEST_CASE("GPIO: Set enable interrupt","[Unit Test: GPIO]"){

   my_gpio.interface.attachInterrupt(&my_gpio.interface,&gpio_callback,CHANGE);
   my_gpio.interface.enableInterrupt(&my_gpio.interface);


}

TEST_CASE("GPIO: Set disable interrupt","[Unit Test: GPIO]"){

   
   my_gpio.interface.disableInterrupt(&my_gpio.interface);


}

   


   
