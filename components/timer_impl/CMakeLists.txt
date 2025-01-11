idf_component_register(SRCS "my_timer.c"
                INCLUDE_DIRS "." "../timer_interface/"
                PRIV_REQUIRES esp_timer)
