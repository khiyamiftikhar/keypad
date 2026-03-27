# esp-matrix-keypad

**PWM-driven matrix keypad driver for ESP-IDF with hardware-assisted simultaneous key detection.**

This component uses **interleaved pulse-width encoding on row lines** and **MCPWM capture on column lines** to detect key presses without traditional scanning. Each row continuously drives a unique PWM waveform, allowing key identification by measuring pulse width on the column.
So multiple key presses on a column are correctly detected

> ⚠️ Note: As with all passive matrix keypads without per-switch diodes, **ghosting may occur** under certain multi-key combinations.

---

## 🚀 Why This Approach?

Unlike traditional row-scanning keypads:

- No row multiplexing loop (no scan delay)
- Multiple keys detected **in parallel**
- Hardware-assisted timing using MCPWM capture
- Supports **multiple keys on the same column** via waveform interleaving
- Lower CPU involvement compared to polling-based designs

---

## How It Works

- Each row outputs a **unique PWM pulse width**  
  (e.g. Row1 = 2000 µs, Row2 = 2400 µs, Row3 = 2800 µs, Row4 = 3200 µs)

- Each column is connected to an **MCPWM capture channel**

- When a key is pressed:
  - The row signal propagates to the column
  - The capture unit measures pulse width
  - The originating row is identified

- Multiple simultaneous presses:
  - **Different columns** → independent capture channels
  - **Same column** → interleaved PWM signals are decoded separately

---

## ⚠️ Row Diodes Are Required

Each row output **must include a series diode** before entering the switch matrix.

### Why this is required

This design allows multiple PWM signals to share a column wire.

When multiple keys in the same column are pressed:

Without diodes:
- Row GPIOs **drive into each other**
- PWM signals become **distorted or collapse**
- Multiple keys on the same column **cannot be reliably detected**
- Risk of **GPIO damage** due to contention

With diodes:
- Row signals are **electrically isolated**
- Multiple PWM waveforms can **coexist on the same column**
- Capture decoding works correctly
- GPIOs are protected

### Important distinction

- Row diodes → required for **correct operation and signal integrity** ✅  
- Row diodes → do **not eliminate ghosting** ❌  
- Per-switch diodes → required for **full NKRO (no ghosting)** ✅  

---

## Hardware Requirements

### Recommended Diodes

| Component | Part Number | Package |
|----------|------------|--------|
| Signal diode | 1N4148 | DO-35 |
| SMD version | 1N4148W | SOD-123 |
| Low-drop option | BAT43 | DO-35 |

---

### Column Pull-Down

Each column GPIO should have a **pull-down resistor**:

- Internal (~45kΩ) → acceptable  
- External 10kΩ → recommended for cleaner signals  

---

## 🔌 Full Matrix Wiring Diagram

```
                         1N4148
Row1 GPIO (2000µs PWM) ──>|──┬──[btn]──┬──[btn]──┬──[btn]──┬──[btn]──┐
                             |          |          |          |       |
Row2 GPIO (2400µs PWM) ──>|──┤  [btn]──┤  [btn]──┤  [btn]──┤  [btn]──┤
                             |          |          |          |       |
Row3 GPIO (2800µs PWM) ──>|──┤  [btn]──┤  [btn]──┤  [btn]──┤  [btn]──┤
                             |          |          |          |       |
Row4 GPIO (3200µs PWM) ──>|──┘  [btn]──┘  [btn]──┘  [btn]──┘  [btn]──┘
                                  |          |          |          |
                               Col1 GPIO  Col2 GPIO  Col3 GPIO  Col4 GPIO
                              (Capture)  (Capture)  (Capture)  (Capture)
                              
```

---

## Software Setup

### CMake

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp-pwm-keypad
)
```

---

### Usage Example

```c
#include "keypad.h"

void keyPadHandler(key_event_t event, keypad_event_data_t *evt_data)
{
    ESP_LOGI(TAG, "event=%-18s  key='%c'  timestamp=%lums",
             key_event_to_str(event),
             (char)evt_data->key_id,
             (unsigned long)evt_data->time_stamp);
}

void app_main(void)
{
    uint8_t row_gpios[] = { 12, 13, 14, 15 };
    uint8_t col_gpios[] = { 18, 19, 22, 23 };

    keypad_config_t config = {
    .auto_repeat_disable=true,  //Will not generate repeat events (when button is kept pressed beyond long press)
    .max_simultaneous_keys=5,
    .cb         = keyPadHandler,
    .col_gpios  = col_gpios,
    .row_gpios  = row_gpios,
    .total_cols = 4,
    .total_rows = 4,
    .long_press_duration_us=2000000,      //2000000 us so 2s
    .repeat_press_duration_us=3000000,      // 1 second
    .keymap     = (uint8_t[]){
                    '1','2','3','A',
                    '4','5','6','B',
                    '7','8','9','C',
                    '*','0','#','D'
                  }
        };


    keypad_interface_t *keypad;
    ESP_ERROR_CHECK(keypadCreate(&config, &keypad));
}
```

---

## Configuration Reference

| Field | Description |
|------|------------|
| keymap | Row-major string (`rows × cols` length) |
| row_gpios | Row output GPIOs (PWM) |
| col_gpios | Column input GPIOs (capture) |
| max_simultaneous_keys | Max tracked keys |
| auto_repeat_disable | Disable repeat events |
| long_press_ms | Long press threshold |
| repeat_interval_ms | Repeat interval |
| cb | Event callback |
| context | User pointer |

---

## Events

```c
typedef enum {
    KEY_PRESSED,
    KEY_LONG_PRESSED,
    KEY_REPEATED,
    KEY_RELEASED,
} key_event_t;
```

---

## Multi-Key Behaviour and Limitations

- Multiple keys across columns → fully supported  
- Multiple keys in same column → supported (requires row diodes)  
- No scanning delay → all rows active simultaneously  

⚠️ However:

- Ghosting may occur in standard matrix wiring
- This is a **hardware limitation**, not a software issue

---

## ⚠️ Ghosting (Hardware Limitation)

Ghosting occurs when unintended electrical paths form through pressed switches.

### Example

Pressed keys:
- (Row1, Col3)
- (Row4, Col1)
- (Row4, Col3)

This creates a path:

```
Row1 → Col3 → Row4 → Col1
```

Result:
- (Row1, Col1) appears falsely pressed

### Important

- PWM decoding **cannot distinguish ghost paths**
- The signal appears electrically valid

### How to eliminate ghosting

- Add **one diode per switch** (not just per row)

---

## MCPWM Capture Resources

| Chip | Channels |
|------|---------|
| ESP32 | 6 |
| ESP32-S3 | 6 |
| ESP32-C3 | 3 |

---

## License

MIT