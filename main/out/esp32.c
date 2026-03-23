#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static inline void __mp_delay_ms(int32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

typedef struct { uint8_t* ptr; size_t size; } __Slice_uint8_t;

typedef enum {
  PinMode_INPUT = 1,
  PinMode_OUTPUT = 3,
  PinMode_INPUT_PULLUP = 5,
  PinMode_INPUT_PULLDOWN = 6,
} PinMode;

typedef enum {
  PinLevel_LOW = 0,
  PinLevel_HIGH = 1,
} PinLevel;

void main__app_main(void);
void gpio__pin_mode(int32_t pin, PinMode mode);
static inline void gpio__digital_write(int32_t pin, PinLevel value);
static inline PinLevel gpio__digital_read(int32_t pin);

const int32_t main__BLINK_PIN = 2;
const int32_t main__BLINK_PERIOD = 1000;

void main__app_main(void) {
  gpio__pin_mode(main__BLINK_PIN, PinMode_OUTPUT);
  PinLevel led = PinLevel_LOW;
  while (true) {
    gpio__digital_write(main__BLINK_PIN, led);
    if ((led == PinLevel_LOW)) {
      (led = PinLevel_HIGH);
    } else {
      (led = PinLevel_LOW);
    }
    __mp_delay_ms(main__BLINK_PERIOD);
  }
}

void gpio__pin_mode(int32_t pin, PinMode mode) {
  gpio_reset_pin(pin);
  if ((mode == PinMode_INPUT_PULLUP)) {
    gpio_set_direction(pin, ((int32_t)(PinMode_INPUT)));
    gpio_set_pull_mode(pin, 0);
  } else if ((mode == PinMode_INPUT_PULLDOWN)) {
    gpio_set_direction(pin, ((int32_t)(PinMode_INPUT)));
    gpio_set_pull_mode(pin, 1);
  } else {
    gpio_set_direction(pin, ((int32_t)(mode)));
  }
}

static inline void gpio__digital_write(int32_t pin, PinLevel value) {
  gpio_set_level(pin, ((int32_t)(value)));
}

static inline PinLevel gpio__digital_read(int32_t pin) {
  return ((PinLevel)(gpio_get_level(pin)));
}

void app_main(void) { main__app_main(); }

