#ifndef INIT_H
#define INIT_H

#include "esp_err.h"
#include <stdint.h>
#include <string.h>
#include <esp_log.h>
#include "matrix_keypad_nkro.h"
#include "my_timer.h"
#include "pool_queue.h"
#include "interleaved_pwm.h"
#include "keypad_button.h"
#include "scan_manager.h"



esp_err_t configKeypadButtons(button_interface_t** buttons,                               uint8_t* keymap,
                                uint8_t total_buttons,
                                pool_alloc_interface_t* timer_pool,
                                uint32_t prober_time_period,
                                buttonCallBack buttonEventHandler,
                                void* context);


esp_err_t configKeypadOutput(interleaved_pwm_interface_t** self,uint8_t* output_gpio,uint32_t* pulse_widths,uint8_t total_outputs);
esp_err_t configKeypadTimers(timer_interface_t **timers,  uint8_t total_timers,  void *context,buttonCallBack handler);

esp_err_t configTimerPool(pool_alloc_interface_t** pool ,timer_interface_t** timers,uint8_t total_objects);

/// @brief Configure the scanner that reads the inputs of matrix keypad
/// The self pointer is double, so that context contains the address of scanner member and thus can extract keypad_dev_t self from context
/// @param input_gpio   The list of gpio associated
/// @param total_inputs Total gpios
/// @param total_outputs Total outputs of matrix keypad, each with different pwm signal and thus different signal
/// @return 
esp_err_t configKeypadInput(scanner_interface_t** self,uint8_t* input_gpio,uint8_t total_inputs,uint8_t total_outputs,uint32_t* pulse_widths_us,scannerCallBack handler,void* context);
#endif