file(GLOB_RECURSE KEYPAD_SRCS
    src/*.c
    private/*.c
)

idf_component_register(
    SRCS ${KEYPAD_SRCS}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS
        private/init_privs/
        private/keypad-button/keypad-button/
        private/pool_allocator/pool_alloc_interface/
        private/pool_allocator/pool_alloc_queue_impl/
        private/prober/interleaved-pwm/
        private/prober/pwm-line/
        private/scanner/pulse-decoder/
        private/scanner/scan-manager/
        private/timer/timer_interface/
        private/timer/timer_impl/

    PRIV_REQUIRES
        esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_timer
)