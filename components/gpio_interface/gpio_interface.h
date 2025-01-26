#ifndef GPIO_INTERFACE_H
#define GPIO_INTERFACE_H

#include <stdint.h>

// Enum for pin modes (still using Arduino-style naming)
typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_INPUT_OUTPUT

} gpio_mode_t;

// Enum for interrupt modes
typedef enum {
    GPIO_INTERRUPT_MODE_RISING=1,
    GPIO_INTERRUPT_MODE_FALLING,
    GPIO_INTERRUPT_MODE_CHANGE
} interrupt_mode;




typedef enum {
    GPIO_EVENT_NEGEDGE,
    GPIO_EVENT_POSEDGE,
} gpio_event_t;

// Forward declaration of the GPIO interface struct

// GPIO interface struct
typedef struct gpio_interface {
    int (*gpioMode)(struct gpio_interface* self, gpio_mode_t mode);   // Use GPIO number
    int (*digitalWrite)(struct gpio_interface* self, uint8_t value);  // Use GPIO number
    int (*digitalRead)(struct gpio_interface* self);  // Use GPIO number
    int (*attachInterrupt)(struct gpio_interface* self, void (*callback)(gpio_event_t), interrupt_mode mode);  // Use GPIO number
    int (*detachInterrupt)(struct gpio_interface* self);  // Use GPIO number
    int (*enableInterrupt)(struct gpio_interface* self);
    int (*disableInterrupt)(struct gpio_interface* self);

}gpio_interface_t;





#endif // GPIO_INTERFACE_H
