file(GLOB_RECURSE KEYPAD_SRCS
    src/*.c
    private/*.c
)

idf_component_register(
    SRCS ${KEYPAD_SRCS}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS
        private/keypad-button/
        private/pool_allocator/pool_alloc_interface/
        private/pool_allocator/pool_alloc_queue_impl/
        private/scan_line/capture-line-interface/
        private/scan_line/pwm-capture/
        private/scan_line/scan-manager/
        private/scan_line/scanner-common
        private/pwm-line/probe-manager/
        private/pwm-line/pwm_line
        private/timer_interface/
        private/timer_impl/
)