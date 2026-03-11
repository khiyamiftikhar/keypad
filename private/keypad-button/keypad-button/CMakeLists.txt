idf_component_register(SRCS "keypad_button.c"
                        INCLUDE_DIRS "." "../../../pool-allocator/components/pool_alloc_interface"
                        PRIV_REQUIRES timer_impl
                        )