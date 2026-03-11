idf_component_register(SRCS "pwm_capture.c"
                        INCLUDE_DIRS "."
                        REQUIRES  esp_driver_mcpwm capture-line-interface scanner-common)