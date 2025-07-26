
idf_component_register(SRCS "keypad_dev.c"
                        INCLUDE_DIRS "." 
                        PRIV_REQUIRES "timer_impl" "pool_alloc_queue_impl" "scan-manager" "probe-manager" "keypad-button")