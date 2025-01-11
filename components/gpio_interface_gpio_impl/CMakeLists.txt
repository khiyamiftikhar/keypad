idf_component_register(SRCS "my_gpio.c"
                        INCIDE_DIRS "." "../gpio_interface"
                        PRIV_REQUIRES "esp_driver_gpio")