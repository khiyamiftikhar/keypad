idf_component_register(SRCS "scan_manager.c"
                       INCLUDE_DIRS "."
                        PRIV_REQUIRES "pwm-capture" 
                        REQUIRES scanner-common)