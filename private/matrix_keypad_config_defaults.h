#ifndef MATRIX_KEYPAD_CONFIG_DEFAULTS_H
#define MATRIX_KEYPAD_CONFIG_DEFAULTS_H

/* -----------------------------------------------------------
 * Fallback defaults for Kconfig options
 *
 * These are used when the component is compiled without
 * menuconfig or when sdkconfig does not define them.
 * ----------------------------------------------------------- */

#ifndef CONFIG_MATRIX_KEYPAD_MAX_KEYPADS
#define CONFIG_MATRIX_KEYPAD_MAX_KEYPADS 1
#endif

#ifndef CONFIG_MATRIX_KEYPAD_MAX_SIMULTANEOUS_KEYS
#define CONFIG_MATRIX_KEYPAD_MAX_SIMULTANEOUS_KEYS 4
#endif

#ifndef CONFIG_MATRIX_KEYPAD_MAX_ROWS
#define CONFIG_MATRIX_KEYPAD_MAX_ROWS 4
#endif

#ifndef CONFIG_MATRIX_KEYPAD_MAX_COLS
#define CONFIG_MATRIX_KEYPAD_MAX_COLS 4
#endif


/* ───────── Button Behaviour Defaults ───────── */

#ifndef CONFIG_MATRIX_KEYPAD_DEBOUNCE_MS
#define CONFIG_MATRIX_KEYPAD_DEBOUNCE_MS 20
#endif

#ifndef CONFIG_MATRIX_KEYPAD_LONG_PRESS_MS
#define CONFIG_MATRIX_KEYPAD_LONG_PRESS_MS 1000
#endif

#ifndef CONFIG_MATRIX_KEYPAD_REPEAT_PRESS_MS
#define CONFIG_MATRIX_KEYPAD_REPEAT_PRESS_MS 500
#endif


#endif /* MATRIX_KEYPAD_CONFIG_DEFAULTS_H */