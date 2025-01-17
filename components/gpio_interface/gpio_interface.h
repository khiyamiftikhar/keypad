#ifndef GPIO_INTERFACE_H
#define GPIO_INTERFACE_H

#include <stdint.h>

// Enum for pin modes (still using Arduino-style naming)
typedef enum {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN,
    INPUT_OUTPUT

} gpio_mode;

// Enum for interrupt modes
typedef enum {
    RISING=1,
    FALLING,
    CHANGE
} interrupt_mode;




typedef enum {
    NEGEDGE,
    POSEDGE,
} gpio_event_t;

// Forward declaration of the GPIO interface struct

// GPIO interface struct
typedef struct gpio_interface {
   int (*gpioMode)(struct gpio_interface* self, gpio_mode mode);   // Use GPIO number
    int (*digitalWrite)(struct gpio_interface* self, uint8_t value);  // Use GPIO number
    int (*digitalRead)(struct gpio_interface* self);  // Use GPIO number
    int (*attachInterrupt)(struct gpio_interface* self, void (*callback)(gpio_event_t), interrupt_mode mode);  // Use GPIO number
    int (*detachInterrupt)(struct gpio_interface* self);  // Use GPIO number
    int (*enableInterrupt)(struct gpio_interface* self);
    int (*disableInterrupt)(struct gpio_interface* self);

}gpio_interface_t;





#endif // GPIO_INTERFACE_H
