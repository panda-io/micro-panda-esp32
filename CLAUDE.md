# micro-panda-esp32

ESP32/ESP-IDF wrapper library for [Micro-Panda](https://github.com/panda-io/micro-panda-dart).
Provides Arduino-like APIs over ESP-IDF so Micro-Panda programs can run on ESP32 hardware.

## Project layout

```
main/
  mpd.yaml          Micro-Panda project config (platform: esp32, entry_fn: app_main)
  CMakeLists.txt    IDF component — runs `mpd gen esp32` at configure time
  src/
    main.mpd        Application entry point (fun app_main)
    gpio.mpd        GPIO wrapper
    rtos.mpd        FreeRTOS basics (delay, delay_ms)
  out/
    esp32.c         Generated C — committed so IDF can build without mpd installed
```

## Build

**Generate C only** (after editing .mpd sources):
```sh
mpd -C main gen esp32
```

**Full IDF build** (from project root):
```sh
idf.py build
```

CMake runs `mpd gen esp32` automatically at configure time, so a plain `idf.py build`
regenerates C whenever `src/*.mpd` or `mpd.yaml` change.

**Flash and monitor:**
```sh
idf.py flash monitor
```

## Compiler

Micro-Panda compiler lives at `/Users/sang/Dev/panda-io/micro-panda-dart/`.
Rebuild after compiler changes:
```sh
cd /Users/sang/Dev/panda-io/micro-panda-dart && ./install.sh
```

## Conventions

### Module structure

Each wrapper module has two sections:

1. **Private ESP-IDF bindings** — `@extern` functions prefixed with `_`, not part of the public API.
2. **Public Arduino-like API** — clean, typed functions users call directly.

Example (`gpio.mpd`):
```
@extern("gpio_set_level")
fun _gpio_set_level(pin: i32, level: i32)   // private

@inline
fun digital_write(pin: i32, value: PinLevel) // public
    _gpio_set_level(pin, i32(value))
```

### Macros → `@raw` helpers

ESP-IDF C macros (e.g. `pdMS_TO_TICKS`, `portTICK_PERIOD_MS`) cannot be bound with
`@extern`. Wrap them in a `static inline` C helper via `@raw`, then bind that:

```
@raw("static inline void __mp_delay_ms(int32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }")
@extern("__mp_delay_ms")
fun delay_ms(ms: i32)
```

### Enum sentinel values

`PinMode.INPUT_PULLUP` and `INPUT_PULLDOWN` use values (5, 6) that don't exist in
`gpio_mode_t`. They are sentinels handled entirely inside `pin_mode` — do not pass them
directly to IDF functions.

## Modules

### `gpio.mpd`

| Function | Description |
|---|---|
| `pin_mode(pin, PinMode)` | Reset pin and set direction + pull |
| `digital_write(pin, PinLevel)` | Set output level |
| `digital_read(pin) PinLevel` | Read input level |

| Enum | Values |
|---|---|
| `PinMode` | `INPUT=1`, `OUTPUT=3`, `INPUT_PULLUP=5`, `INPUT_PULLDOWN=6` |
| `PinLevel` | `LOW=0`, `HIGH=1` |

### `rtos.mpd`

| Function | Description |
|---|---|
| `delay(ticks)` | Delay by raw FreeRTOS ticks (`vTaskDelay`) |
| `delay_ms(ms)` | Delay by milliseconds (uses `pdMS_TO_TICKS` — correct for any tick rate) |

## Roadmap

- `log.mpd` — `esp_log` (`log_i`, `log_w`, `log_e`)
- `adc.mpd` — analog read
- `ledc.mpd` — PWM output
- `uart.mpd` — serial read/write
- `i2c.mpd` — I2C master
- `nvs.mpd` — key-value store in flash
- `wifi.mpd` — WiFi connect/disconnect
