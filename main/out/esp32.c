#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static inline void __mp_delay_ms(int32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
#include <stdio.h>

typedef void (*__Fn_void_uint8_t)(uint8_t);

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
static void console___write_byte_default(uint8_t b);
void console__init(__Fn_void_uint8_t write_byte);
static inline void console__write_byte(uint8_t b);
void console__println(void);
void console__print_str(__Slice_uint8_t s);
void console__print_bool(bool v);
void console__print_u64(uint64_t v);
void console__print_u32(uint32_t v);
void console__print_u16(uint16_t v);
void console__print_u8(uint8_t v);
void console__print_i64(int64_t v);
void console__print_i32(int32_t v);
void console__print_i16(int16_t v);
void console__print_i8(int8_t v);
void console__print_float(float v, uint32_t decimals);
void console__print_fixed(int32_t v, uint32_t decimals);

const int32_t main__BLINK_PIN = 2;
const int32_t main__BLINK_PERIOD = 1000;
int32_t main__count = 0;
static __Fn_void_uint8_t console___write_byte = console___write_byte_default;

void main__app_main(void) {
  console__print_str((__Slice_uint8_t){(uint8_t*)"micro-panda esp32 ready\n", sizeof("micro-panda esp32 ready\n") - 1});
  gpio__pin_mode(main__BLINK_PIN, PinMode_OUTPUT);
  PinLevel led = PinLevel_LOW;
  while (true) {
    gpio__digital_write(main__BLINK_PIN, led);
    if ((led == PinLevel_LOW)) {
      (led = PinLevel_HIGH);
      console__print_str((__Slice_uint8_t){(uint8_t*)"LED ON  count=", sizeof("LED ON  count=") - 1});
    } else {
      (led = PinLevel_LOW);
      console__print_str((__Slice_uint8_t){(uint8_t*)"LED OFF count=", sizeof("LED OFF count=") - 1});
    }
    console__print_i32(main__count);
    console__println();
    (main__count += 1);
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

static void console___write_byte_default(uint8_t b) {
  putchar(b);
}

void console__init(__Fn_void_uint8_t write_byte) {
  (console___write_byte = write_byte);
}

static inline void console__write_byte(uint8_t b) {
  console___write_byte(b);
}

void console__println(void) {
  console__write_byte(10);
}

void console__print_str(__Slice_uint8_t s) {
  uint32_t i = 0;
  while ((i < s.size)) {
    console__write_byte(s.ptr[i]);
    (i += 1);
  }
}

void console__print_bool(bool v) {
  if (v) {
    console__write_byte('t');
    console__write_byte('r');
    console__write_byte('u');
    console__write_byte('e');
  } else {
    console__write_byte('f');
    console__write_byte('a');
    console__write_byte('l');
    console__write_byte('s');
    console__write_byte('e');
  }
}

void console__print_u64(uint64_t v) {
  uint8_t buf[20];
  int32_t i = 19;
  if ((v == 0)) {
    console__write_byte('0');
    return;
  }
  while ((v > 0)) {
    (buf[i] = ((uint8_t)(((v % 10) + 48))));
    (v = (v / 10));
    (i -= 1);
  }
  int32_t j = (i + 1);
  while ((j < 20)) {
    console__write_byte(buf[j]);
    (j += 1);
  }
}

void console__print_u32(uint32_t v) {
  console__print_u64(((uint64_t)(v)));
}

void console__print_u16(uint16_t v) {
  console__print_u64(((uint64_t)(v)));
}

void console__print_u8(uint8_t v) {
  console__print_u64(((uint64_t)(v)));
}

void console__print_i64(int64_t v) {
  if ((v < 0)) {
    console__write_byte('-');
    console__print_u64(((uint64_t)((((int64_t)(0)) - v))));
  } else {
    console__print_u64(((uint64_t)(v)));
  }
}

void console__print_i32(int32_t v) {
  console__print_i64(((int64_t)(v)));
}

void console__print_i16(int16_t v) {
  console__print_i64(((int64_t)(v)));
}

void console__print_i8(int8_t v) {
  console__print_i64(((int64_t)(v)));
}

void console__print_float(float v, uint32_t decimals) {
  float abs = v;
  if ((v < 0.0f)) {
    console__write_byte('-');
    (abs = (0.0f - v));
  }
  int32_t int_part = ((int32_t)(abs));
  console__print_i32(int_part);
  if ((decimals > 0)) {
    console__write_byte('.');
    float frac = (abs - ((float)(int_part)));
    uint32_t i = 0;
    while ((i < decimals)) {
      (frac = (frac * 10.0f));
      int32_t digit = ((int32_t)(frac));
      console__write_byte(((uint8_t)((digit + 48))));
      (frac = (frac - ((float)(digit))));
      (i += 1);
    }
  }
}

void console__print_fixed(int32_t v, uint32_t decimals) {
  uint32_t abs = 0;
  if ((v < 0)) {
    console__write_byte('-');
    (abs = ((uint32_t)((0 - v))));
  } else {
    (abs = ((uint32_t)(v)));
  }
  console__print_u32((abs >> 16));
  if ((decimals > 0)) {
    console__write_byte('.');
    uint32_t frac = (abs & 0xFFFF);
    uint32_t i = 0;
    while ((i < decimals)) {
      (frac *= 10);
      console__write_byte(((uint8_t)(((frac >> 16) + 48))));
      (frac = (frac & 0xFFFF));
      (i += 1);
    }
  }
}

void app_main(void) { main__app_main(); }

