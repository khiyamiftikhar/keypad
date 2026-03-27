# matrix-keypad-pwm

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue)
![Espressif Component Registry](https://img.shields.io/badge/Espressif-Component%20Registry-orange)
![License](https://img.shields.io/badge/license-MIT-green)

ESP-IDF component for **matrix keypads with hardware-assisted simultaneous key detection**.

Instead of scanning rows one at a time, each row continuously drives a unique PWM pulse width. Column lines are connected to MCPWM capture channels. Any keypress is identified by measuring the pulse width that arrives on the column — no polling loop, no scan delay.

> ⚠️ As with all passive matrix keypads without per-switch diodes, **ghosting may occur** under certain multi-key combinations. See the Ghosting section below.

---

## Features

- **No row scanning** — all rows drive simultaneously
- **Multiple keys detected in parallel** across columns
- **Same-column multi-key** — interleaved PWM signals decoded independently
- Hardware-assisted timing via MCPWM capture peripheral
- Configurable **long press**, **repeat**, and **simultaneous key** limits
- **Callback-based API** — key pressed, long pressed, repeated, released
- Single instance, up to **6 column lines** (hardware limit)

---

## Installation

### Using ESP-IDF Component Manager (Recommended)

```bash
idf.py add-dependency "embedblocks/matrix-keypad-pwm^0.1.0"
```

Or in your project's `idf_component.yml`:

```yaml
dependencies:
  embedblocks/matrix-keypad-pwm: "^0.1.0"
```

---

## How It Works

Each row GPIO outputs a continuous PWM signal with a unique pulse width:

```
Row1 GPIO → 2000 µs pulse width
Row2 GPIO → 2400 µs pulse width
Row3 GPIO → 2800 µs pulse width
Row4 GPIO → 3200 µs pulse width
```

When a key is pressed, the row signal reaches the column wire. The MCPWM capture channel on that column measures the pulse width and identifies which row it came from:

```
Row GPIO ──>|──[btn]──┐
                      │  Column wire
                      ├── MCPWM capture
                      │
            measure pulse width → match row → fire callback
```

### Multiple keys on different columns

Each column has its own independent capture channel. Presses are detected fully in parallel:

```
Col1 capture  →  Row2 signal  →  KEY (Row2, Col1) ✓
Col3 capture  →  Row4 signal  →  KEY (Row4, Col3) ✓   simultaneously
```

### Multiple keys on the same column

Two pressed buttons in the same column produce **interleaved PWM** on that wire. Both waveforms are captured and decoded independently:

```
Col1 wire:  ─┐  ┌──┐  ┌─┐  ┌──┐  ┌─
             └──┘  └──┘ └──┘  └──┘

             ←2000µs→←2400µs→← ...

Decoded:   Row1 pulse ✓    Row2 pulse ✓
```

---

## ⚠️ Row Diodes Are Required

Each row output **must have a series diode** (anode toward GPIO, cathode into matrix) before entering the switch matrix.

```
Row GPIO ──>|── switch matrix
```

### Why

When multiple keys in the same column are pressed simultaneously, the row GPIOs share a common column wire. Without diodes, a HIGH row driver contends directly against a LOW row driver through the closed switches — distorting the PWM signal and risking GPIO damage over time.

With diodes, each row signal is electrically isolated. Multiple PWM waveforms coexist on the column wire without contention and are decoded correctly.

### Recommended Parts

| Component | Part Number | Package |
|-----------|-------------|---------|
| Signal diode | **1N4148** | DO-35 (through-hole) |
| SMD version | **1N4148W** | SOD-123 |
| Low-drop option | **BAT43** | DO-35 (0.3V drop vs 0.6V) |

### Column Pull-Down

Each column capture GPIO needs a pull-down to GND. The diodes only pull the column HIGH — when no button is pressed the line must return to LOW cleanly.

- Internal ESP32 pull-down (~45 kΩ) → acceptable
- External 10 kΩ to GND → recommended for cleaner edges

---

## Wiring Diagram

```
                         1N4148
Row1 GPIO (2000µs PWM) ──>|──┬──[btn]──┬──[btn]──┬──[btn]──┬──[btn]──┐
                             |          |          |          |         |
Row2 GPIO (2400µs PWM) ──>|──┤  [btn]──┤  [btn]──┤  [btn]──┤  [btn]──┤
                             |          |          |          |         |
Row3 GPIO (2800µs PWM) ──>|──┤  [btn]──┤  [btn]──┤  [btn]──┤  [btn]──┤
                             |          |          |          |         |
Row4 GPIO (3200µs PWM) ──>|──┘  [btn]──┘  [btn]──┘  [btn]──┘  [btn]──┘
                                  |          |          |          |
                               Col1 GPIO  Col2 GPIO  Col3 GPIO  Col4 GPIO
                              (Capture)  (Capture)  (Capture)  (Capture)
```

---

## Usage

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
        .auto_repeat_disable      = false,
        .max_simultaneous_keys    = 3,
        .long_press_duration_us   = 2000000,   /* 2 s  */
        .repeat_press_duration_us = 1000000,   /* 1 s  */
        .cb                       = keyPadHandler,
        .context                  = NULL,
        .row_gpios                = row_gpios,
        .total_rows               = 4,
        .col_gpios                = col_gpios,
        .total_cols               = 4,
        .keymap                   = (uint8_t[]){
                                      '1','2','3','A',
                                      '4','5','6','B',
                                      '7','8','9','C',
                                      '*','0','#','D'
                                    },
    };

    keypad_interface_t *keypad;
    ESP_ERROR_CHECK(keypadCreate(&config, &keypad));
}
```

---

## Configuration Reference

| Field | Type | Description |
|-------|------|-------------|
| `keymap` | `uint8_t *` | Flat row-major array of key characters, length `total_rows × total_cols` |
| `row_gpios` | `uint8_t *` | GPIO numbers for row outputs (PWM drivers) |
| `total_rows` | `uint8_t` | Number of rows |
| `col_gpios` | `uint8_t *` | GPIO numbers for column inputs (MCPWM capture) |
| `total_cols` | `uint8_t` | Number of columns |
| `max_simultaneous_keys` | `uint8_t` | Maximum concurrent keypresses to track |
| `auto_repeat_disable` | `bool` | `true` to suppress repeat events while a key is held |
| `long_press_duration_us` | `uint32_t` | Hold time before `KEY_LONG_PRESSED` fires (µs) |
| `repeat_press_duration_us` | `uint32_t` | Interval between `KEY_REPEATED` events (µs) |
| `cb` | `keypadCallback` | Event callback |
| `context` | `void *` | User pointer passed back in every callback — avoids globals |

---

## Events

```c
typedef enum {
    KEY_PRESSED,
    KEY_LONG_PRESSED,
    KEY_REPEATED,
    KEY_RELEASED,
} key_event_t;

typedef struct {
    uint8_t  key_id;            /* character from keymap                     */
    uint8_t  row;               /* 0-based row index                         */
    uint8_t  col;               /* 0-based column index                      */
    uint32_t time_stamp;        /* FreeRTOS tick in ms                       */
    uint32_t hold_duration_ms;  /* time held since KEY_PRESSED (0 on press)  */
} keypad_event_data_t;
```

---

## ⚠️ Ghosting (Hardware Limitation)

Ghosting is a fundamental property of passive matrix keypads. **No software can eliminate it** — only per-switch diodes (one per button, custom PCB) can.

### How it happens

Consider two buttons pressed on Row2 and one on Row1:

```
         Col1     Col2     Col3     Col4
Row1:     ?                 X              ← only Col3 physically pressed
Row2:     X                 X              ← Col1 and Col3 physically pressed
Row3:
Row4:
```

```
         Col1          Col3
          |             |
Row1 ──>|──────────────[sw]─── Col3 wire  (Row1-Col3: real press ✅)
                        |
                        │ signal leaks down Col3
                        ▼
Row2 ──>|──[sw]────────[sw]
         |              |
         │ ◄────────────┘  signal travels LEFT along Row2 wire
         │
         ▼
        Col1 wire
         |
         ▼
    capture measures Row1 pulse width
    → Row1-Col1 reported as PRESSED (GHOST ❌)
```
Row1's signal leaks through the closed Row2-Col3 switch onto the Row2 wire, then escapes to Col1 through the closed Row2-Col1 switch. The capture channel on Col1 sees a valid Row1 pulse width and cannot distinguish it from a real press.

### What row diodes fix vs what they do not

| Protection | Row diodes | Per-switch diodes |
|------------|-----------|-------------------|
| GPIO driver contention | ✅ | ✅ |
| GPIO damage from contention | ✅ | ✅ |
| Ghosting | ❌ | ✅ |

Row diodes block Row signals from backdriving each other's GPIO outputs — but the ghost path travels entirely through the switch matrix wire between the diode and the switches, which the row diode cannot block.

---

## Chip Support

| Chip | Status | Capture Channels |
|------|--------|-----------------|
| ESP32 | ✅ Tested | 6 (2 groups × 3) |
| ESP32-S3 | ⚠️ Expected to work | 6 (2 groups × 3) |
| ESP32-C3 | ⚠️ Expected to work | 3 (1 group × 3) |
| ESP32-H2 | ⚠️ Expected to work | 3 (1 group × 3) |

A 4×4 keypad (4 columns) fits on all variants. A 4×6 keypad (6 columns) requires ESP32 or S3.

---

## Limitations

- **Single instance** — only one keypad per application
- Maximum **6 column lines** (MCPWM hardware limit)
- Ghosting is a hardware property of passive matrix keypads — see Ghosting section above
- Only ESP32 has been fully validated; other chips expected to work based on driver compatibility

---

## License

MIT