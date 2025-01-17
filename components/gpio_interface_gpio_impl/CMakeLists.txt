idf_component_register(SRCS "my_gpio.c"
                        INCLUDE_DIRS "." "../gpio_interface/"
                        PRIV_REQUIRES "esp_driver_gpio")