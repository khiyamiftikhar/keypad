#ifndef GPIO_INTERFACE_H
#define GPIO_INTERFACE_H

#include <stdint.h>

// Enum for pin modes (still using Arduino-style naming)
typedef enum {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN
} gpio_mode;

// Enum for interrupt modes
typedef enum {
    RISING,
    FALLING,
    CHANGE
} interrupt_mode;


typedef enum {
    LOW,
    HIGH,
    RISING,
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
    void (*gpioMode)(gpio_interface_t* self, uint8_t gpio, gpio_mode mode);   // Use GPIO number
    void (*digitalWrite)(gpio_interface_t* self, uint8_t gpio, uint8_t value);  // Use GPIO number
    uint8_t (*digitalRead)(gpio_interface_t* self, uint8_t gpio);  // Use GPIO number
    void (*attachInterrupt)(gpio_interface_t* self, uint8_t gpio, void (*callback)(gpio_event_t), interrupt_mode mode,void* context);  // Use GPIO number
    void (*detachInterrupt)(gpio_interface_t* self, uint8_t gpio);  // Use GPIO number
}gpio_interface_t;





#endif // GPIO_INTERFACE_H
