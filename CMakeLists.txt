file(GLOB_RECURSE KEYPAD_SRCS
    src/*.c
    private/*.c
)

idf_component_register(
    SRCS ${KEYPAD_SRCS}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS
        private/keypad-button/keypad-button
        private/pool_allocator/pool_alloc_interface/
        private/pool_allocator/pool_alloc_queue_impl/
        private/scan-line/capture-line-interface/
        private/scan-line/pwm-capture/
        private/scan-line/scan-manager/
        private/scan-line/scanner-common/
        private/pwm-line/probe-manager/
        private/pwm-line/pwm_line_interface/
        private/pwm-line/pwm-line/
        private/timer/timer_interface/
        private/timer/timer_impl/

    PRIV_REQUIRES
        esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_timer
)