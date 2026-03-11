idf_component_register(SRCS "pwm_line.c"
                        INCLUDE_DIRS "." "../pwm_line_interface/"
                        PRIV_REQUIRES esp_driver_ledc esp_driver_gpio
                        REQUIRES pwm_line_interface )