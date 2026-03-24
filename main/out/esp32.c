#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

static inline void __mp_pwm_timer_init(int32_t freq_hz, int32_t timer) {
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = (ledc_timer_t)timer,
        .freq_hz         = (uint32_t)freq_hz,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
}
static inline void __mp_pwm_channel_init(int32_t pin, int32_t ch, int32_t timer) {
    ledc_channel_config_t c = {
        .gpio_num   = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = (ledc_channel_t)ch,
        .timer_sel  = (ledc_timer_t)timer,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&c);
}
static inline void __mp_pwm_set_duty(int32_t ch, int32_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, (uint32_t)duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch);
}
static inline void __mp_pwm_stop(int32_t ch) {
    ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, 0);
}
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static inline void     __mp_delay_ms(int32_t ms)   { vTaskDelay(pdMS_TO_TICKS(ms)); }
static inline int32_t  __mp_time_ms(void)           { return (int32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS); }
static inline void     __mp_task_create(void (*fn)(void), int32_t stack, int32_t priority) {
    xTaskCreate((TaskFunction_t)(void*)fn, "", (uint32_t)stack, NULL, (UBaseType_t)priority, NULL);
}
static inline void     __mp_task_exit(void)         { vTaskDelete(NULL); }
#include <stdio.h>
static inline int32_t __mp_float_to_bits(float f) { int32_t v; __builtin_memcpy(&v, &f, 4); return v; }
static inline float __mp_bits_to_float(int32_t v) { float f; __builtin_memcpy(&f, &v, 4); return f; }

typedef void (*__Fn_void_void)(void);
typedef void (*__Fn_void_uint8_t)(uint8_t);

typedef struct { uint8_t* ptr; size_t size; } __Slice_uint8_t;
typedef struct { int32_t* ptr; size_t size; } __Slice_int32_t;

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

typedef enum {
  PwmTimer_TIMER0 = 0,
  PwmTimer_TIMER1 = 1,
  PwmTimer_TIMER2 = 2,
  PwmTimer_TIMER3 = 3,
} PwmTimer;

void main__blink(void);
void main__fade(void);
void main__task(void);
void main__app_main(void);
void gpio__pin_mode(int32_t pin, PinMode mode);
static inline void gpio__digital_write(int32_t pin, PinLevel value);
static inline PinLevel gpio__digital_read(int32_t pin);
static int32_t pwm___find(int32_t pin);
static int32_t pwm___alloc(int32_t pin);
void pwm__pwm_timer(PwmTimer timer, int32_t freq);
void pwm__pwm_attach(int32_t pin, PwmTimer timer);
void pwm__pwm_start(int32_t pin, int32_t duty);
void pwm__pwm_stop(int32_t pin);
static inline void log___esc(__Slice_uint8_t seq);
void log__info(__Slice_uint8_t msg);
void log__warn(__Slice_uint8_t msg);
void log__error(__Slice_uint8_t msg);
void log__info_args(__Slice_uint8_t fmt, __Slice_int32_t args);
void log__warn_args(__Slice_uint8_t fmt, __Slice_int32_t args);
void log__error_args(__Slice_uint8_t fmt, __Slice_int32_t args);
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
void console__print_float(float v);
void console__print_fixed(int32_t v);
bool string__equals(__Slice_uint8_t a, __Slice_uint8_t b);
bool string__starts_with(__Slice_uint8_t s, __Slice_uint8_t prefix);
bool string__ends_with(__Slice_uint8_t s, __Slice_uint8_t suffix);
int32_t string__index_of(__Slice_uint8_t s, uint8_t c);
__Slice_uint8_t string__sub(__Slice_uint8_t s, uint32_t start, uint32_t len);
__Slice_uint8_t string__trim_start(__Slice_uint8_t s);
__Slice_uint8_t string__trim_end(__Slice_uint8_t s);
__Slice_uint8_t string__trim(__Slice_uint8_t s);
__Slice_uint8_t string__token(__Slice_uint8_t s, uint32_t start, uint8_t delim);
uint32_t string__skip(__Slice_uint8_t s, uint32_t start, uint8_t delim);
uint32_t string__parse_u32(__Slice_uint8_t s);
int32_t string__parse_i32(__Slice_uint8_t s);
uint32_t string__format_u32(__Slice_uint8_t buf, uint32_t v);
uint32_t string__format_i32(__Slice_uint8_t buf, int32_t v);
static uint32_t string___format_float(__Slice_uint8_t buf, float v);
static uint32_t string___format_fixed(__Slice_uint8_t buf, int32_t v);
static uint32_t string___format_bool(__Slice_uint8_t buf, int32_t v);
__Slice_uint8_t string__format(__Slice_uint8_t text, __Slice_uint8_t buf, __Slice_int32_t args);

const int32_t main__LED_PIN = 2;
const int32_t main__FADE_STEP = 32;
const int32_t main__FADE_DELAY = 20;
const int32_t main__BLINK_MS = 500;
int32_t main__duty = 0;
int32_t main__dir = 1;
static int32_t pwm___pins[6];
static uint8_t log___buf[128];
const int32_t config__PWM_MAX_CHANNELS = 6;
static __Fn_void_uint8_t console___write_byte = console___write_byte_default;

void main__blink(void) {
  gpio__pin_mode(main__LED_PIN, PinMode_OUTPUT);
  int32_t args[1];
  (args[0] = 0);
  while (true) {
    gpio__digital_write(main__LED_PIN, PinLevel_HIGH);
    __mp_delay_ms(main__BLINK_MS);
    gpio__digital_write(main__LED_PIN, PinLevel_LOW);
    __mp_delay_ms(main__BLINK_MS);
    (args[0] = __mp_time_ms());
    log__info_args((__Slice_uint8_t){(uint8_t*)"current time: {0i} ms", sizeof("current time: {0i} ms") - 1}, (__Slice_int32_t){args, 1});
  }
}

void main__fade(void) {
  pwm__pwm_timer(PwmTimer_TIMER0, 5000);
  pwm__pwm_attach(main__LED_PIN, PwmTimer_TIMER0);
  while (true) {
    pwm__pwm_start(main__LED_PIN, main__duty);
    (main__duty += (main__dir * main__FADE_STEP));
    if ((main__duty >= 1023)) {
      (main__duty = 1023);
      (main__dir = (-1));
    }
    if ((main__duty <= 0)) {
      (main__duty = 0);
      (main__dir = 1);
    }
    __mp_delay_ms(main__FADE_DELAY);
  }
}

void main__task(void) {
  int32_t count = 5;
  while ((count > 0)) {
    log__info((__Slice_uint8_t){(uint8_t*)"executing task...", sizeof("executing task...") - 1});
    (count -= 1);
  }
  __mp_task_exit();
}

void main__app_main(void) {
  log__info((__Slice_uint8_t){(uint8_t*)"micro-panda esp32 ready", sizeof("micro-panda esp32 ready") - 1});
  __mp_task_create(main__task, 1024, 1);
  main__blink();
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

static int32_t pwm___find(int32_t pin) {
  int32_t i = 0;
  while ((i < config__PWM_MAX_CHANNELS)) {
    if ((pwm___pins[i] == (pin + 1))) {
      return i;
    }
    (i += 1);
  }
  return (-1);
}

static int32_t pwm___alloc(int32_t pin) {
  int32_t i = 0;
  while ((i < config__PWM_MAX_CHANNELS)) {
    if ((pwm___pins[i] == 0)) {
      (pwm___pins[i] = (pin + 1));
      return i;
    }
    (i += 1);
  }
  return (-1);
}

void pwm__pwm_timer(PwmTimer timer, int32_t freq) {
  __mp_pwm_timer_init(freq, ((int32_t)(timer)));
}

void pwm__pwm_attach(int32_t pin, PwmTimer timer) {
  int32_t ch = pwm___find(pin);
  if ((ch == (-1))) {
    (ch = pwm___alloc(pin));
  }
  if ((ch >= 0)) {
    __mp_pwm_channel_init(pin, ch, ((int32_t)(timer)));
  }
}

void pwm__pwm_start(int32_t pin, int32_t duty) {
  int32_t ch = pwm___find(pin);
  if ((ch >= 0)) {
    __mp_pwm_set_duty(ch, duty);
  }
}

void pwm__pwm_stop(int32_t pin) {
  int32_t ch = pwm___find(pin);
  if ((ch >= 0)) {
    __mp_pwm_stop(ch);
    (pwm___pins[ch] = 0);
  }
}

static inline void log___esc(__Slice_uint8_t seq) {
  console__write_byte(27);
  console__write_byte('[');
  console__print_str(seq);
}

void log__info(__Slice_uint8_t msg) {
  log___esc((__Slice_uint8_t){(uint8_t*)"32m", sizeof("32m") - 1});
  console__print_str((__Slice_uint8_t){(uint8_t*)"[INFO] ", sizeof("[INFO] ") - 1});
  console__print_str(msg);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__println();
}

void log__warn(__Slice_uint8_t msg) {
  log___esc((__Slice_uint8_t){(uint8_t*)"33m", sizeof("33m") - 1});
  console__print_str((__Slice_uint8_t){(uint8_t*)"[WARN] ", sizeof("[WARN] ") - 1});
  console__print_str(msg);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__println();
}

void log__error(__Slice_uint8_t msg) {
  log___esc((__Slice_uint8_t){(uint8_t*)"31m", sizeof("31m") - 1});
  console__print_str((__Slice_uint8_t){(uint8_t*)"[ERROR] ", sizeof("[ERROR] ") - 1});
  console__print_str(msg);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__println();
}

void log__info_args(__Slice_uint8_t fmt, __Slice_int32_t args) {
  log__info(string__format(fmt, (__Slice_uint8_t){log___buf, 128}, args));
}

void log__warn_args(__Slice_uint8_t fmt, __Slice_int32_t args) {
  log__warn(string__format(fmt, (__Slice_uint8_t){log___buf, 128}, args));
}

void log__error_args(__Slice_uint8_t fmt, __Slice_int32_t args) {
  log__error(string__format(fmt, (__Slice_uint8_t){log___buf, 128}, args));
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

void console__print_float(float v) {
  float abs = v;
  if ((v < 0.0f)) {
    console__write_byte('-');
    (abs = (0.0f - v));
  }
  int32_t int_part = ((int32_t)(abs));
  console__print_i32(int_part);
  console__write_byte('.');
  float frac = (abs - ((float)(int_part)));
  uint32_t i = 0;
  while ((i < 4)) {
    (frac = (frac * 10.0f));
    int32_t digit = ((int32_t)(frac));
    console__write_byte(((uint8_t)((digit + 48))));
    (frac = (frac - ((float)(digit))));
    (i += 1);
  }
}

void console__print_fixed(int32_t v) {
  uint32_t abs = 0;
  if ((v < 0)) {
    console__write_byte('-');
    (abs = ((uint32_t)((0 - v))));
  } else {
    (abs = ((uint32_t)(v)));
  }
  console__print_u32((abs >> 16));
  console__write_byte('.');
  uint32_t frac = (abs & 0xFFFF);
  uint32_t i = 0;
  while ((i < 4)) {
    (frac *= 10);
    console__write_byte(((uint8_t)(((frac >> 16) + 48))));
    (frac = (frac & 0xFFFF));
    (i += 1);
  }
}

bool string__equals(__Slice_uint8_t a, __Slice_uint8_t b) {
  if ((a.size != b.size)) {
    return false;
  }
  uint32_t i = 0;
  while ((i < a.size)) {
    if ((a.ptr[i] != b.ptr[i])) {
      return false;
    }
    (i += 1);
  }
  return true;
}

bool string__starts_with(__Slice_uint8_t s, __Slice_uint8_t prefix) {
  if ((prefix.size > s.size)) {
    return false;
  }
  uint32_t i = 0;
  while ((i < prefix.size)) {
    if ((s.ptr[i] != prefix.ptr[i])) {
      return false;
    }
    (i += 1);
  }
  return true;
}

bool string__ends_with(__Slice_uint8_t s, __Slice_uint8_t suffix) {
  if ((suffix.size > s.size)) {
    return false;
  }
  uint32_t offset = (s.size - suffix.size);
  uint32_t i = 0;
  while ((i < suffix.size)) {
    if ((s.ptr[(offset + i)] != suffix.ptr[i])) {
      return false;
    }
    (i += 1);
  }
  return true;
}

int32_t string__index_of(__Slice_uint8_t s, uint8_t c) {
  uint32_t i = 0;
  while ((i < s.size)) {
    if ((s.ptr[i] == c)) {
      return ((int32_t)(i));
    }
    (i += 1);
  }
  return (-1);
}

__Slice_uint8_t string__sub(__Slice_uint8_t s, uint32_t start, uint32_t len) {
  return (__Slice_uint8_t){(s.ptr + start), len};
}

__Slice_uint8_t string__trim_start(__Slice_uint8_t s) {
  uint32_t i = 0;
  while ((i < s.size)) {
    uint8_t c = s.ptr[i];
    if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
      return (__Slice_uint8_t){(s.ptr + i), (s.size - i)};
    }
    (i += 1);
  }
  return (__Slice_uint8_t){s.ptr, 0};
}

__Slice_uint8_t string__trim_end(__Slice_uint8_t s) {
  if ((s.size == 0)) {
    return s;
  }
  uint32_t i = (s.size - 1);
  while ((i > 0)) {
    uint8_t c = s.ptr[i];
    if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
      return (__Slice_uint8_t){s.ptr, (i + 1)};
    }
    (i -= 1);
  }
  uint8_t c = s.ptr[0];
  if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
    return (__Slice_uint8_t){s.ptr, 1};
  }
  return (__Slice_uint8_t){s.ptr, 0};
}

__Slice_uint8_t string__trim(__Slice_uint8_t s) {
  return string__trim_end(string__trim_start(s));
}

__Slice_uint8_t string__token(__Slice_uint8_t s, uint32_t start, uint8_t delim) {
  uint32_t i = start;
  while ((i < s.size)) {
    if ((s.ptr[i] == delim)) {
      return (__Slice_uint8_t){(s.ptr + start), (i - start)};
    }
    (i += 1);
  }
  return (__Slice_uint8_t){(s.ptr + start), (s.size - start)};
}

uint32_t string__skip(__Slice_uint8_t s, uint32_t start, uint8_t delim) {
  uint32_t i = start;
  while ((i < s.size)) {
    if ((s.ptr[i] != delim)) {
      return i;
    }
    (i += 1);
  }
  return s.size;
}

uint32_t string__parse_u32(__Slice_uint8_t s) {
  uint32_t result = 0;
  uint32_t i = 0;
  while ((i < s.size)) {
    uint8_t c = s.ptr[i];
    if (((c < 48) || (c > 57))) {
      return result;
    }
    (result = ((result * 10) + ((uint32_t)((c - 48)))));
    (i += 1);
  }
  return result;
}

int32_t string__parse_i32(__Slice_uint8_t s) {
  if ((s.size == 0)) {
    return 0;
  }
  if ((s.ptr[0] == 45)) {
    uint32_t abs = string__parse_u32((__Slice_uint8_t){(s.ptr + 1), (s.size - 1)});
    return ((int32_t)((-abs)));
  }
  return ((int32_t)(string__parse_u32(s)));
}

uint32_t string__format_u32(__Slice_uint8_t buf, uint32_t v) {
  if ((v == 0)) {
    (buf.ptr[0] = 48);
    return 1;
  }
  uint8_t tmp[10];
  uint32_t len = 0;
  uint32_t n = v;
  while ((n > 0)) {
    (tmp[len] = ((uint8_t)((48 + (n % 10)))));
    (n = (n / 10));
    (len += 1);
  }
  uint32_t i = 0;
  while ((i < len)) {
    (buf.ptr[i] = tmp[((len - 1) - i)]);
    (i += 1);
  }
  return len;
}

uint32_t string__format_i32(__Slice_uint8_t buf, int32_t v) {
  if ((v < 0)) {
    (buf.ptr[0] = 45);
    uint32_t written = string__format_u32((__Slice_uint8_t){(buf.ptr + 1), (buf.size - 1)}, ((uint32_t)((-v))));
    return (written + 1);
  }
  return string__format_u32(buf, ((uint32_t)(v)));
}

static uint32_t string___format_float(__Slice_uint8_t buf, float v) {
  uint32_t bi = 0;
  float abs = v;
  if ((v < 0.0f)) {
    (buf.ptr[bi] = 45);
    (bi += 1);
    (abs = (0.0f - v));
  }
  int32_t int_part = ((int32_t)(abs));
  (bi += string__format_i32((__Slice_uint8_t){(buf.ptr + bi), (buf.size - bi)}, int_part));
  (buf.ptr[bi] = 46);
  (bi += 1);
  float frac = (abs - ((float)(int_part)));
  uint32_t d = 0;
  while ((d < 4)) {
    (frac = (frac * 10.0f));
    int32_t digit = ((int32_t)(frac));
    (buf.ptr[bi] = ((uint8_t)((digit + 48))));
    (bi += 1);
    (frac = (frac - ((float)(digit))));
    (d += 1);
  }
  return bi;
}

static uint32_t string___format_fixed(__Slice_uint8_t buf, int32_t v) {
  uint32_t bi = 0;
  uint32_t abs = 0;
  if ((v < 0)) {
    (buf.ptr[bi] = 45);
    (bi += 1);
    (abs = ((uint32_t)((0 - v))));
  } else {
    (abs = ((uint32_t)(v)));
  }
  (bi += string__format_u32((__Slice_uint8_t){(buf.ptr + bi), (buf.size - bi)}, (abs >> 16)));
  (buf.ptr[bi] = 46);
  (bi += 1);
  uint32_t frac = (abs & 0xFFFF);
  uint32_t d = 0;
  while ((d < 4)) {
    (frac *= 10);
    (buf.ptr[bi] = ((uint8_t)(((frac >> 16) + 48))));
    (bi += 1);
    (frac = (frac & 0xFFFF));
    (d += 1);
  }
  return bi;
}

static uint32_t string___format_bool(__Slice_uint8_t buf, int32_t v) {
  if ((v != 0)) {
    (buf.ptr[0] = 't');
    (buf.ptr[1] = 'r');
    (buf.ptr[2] = 'u');
    (buf.ptr[3] = 'e');
    return 4;
  }
  (buf.ptr[0] = 'f');
  (buf.ptr[1] = 'a');
  (buf.ptr[2] = 'l');
  (buf.ptr[3] = 's');
  (buf.ptr[4] = 'e');
  return 5;
}

__Slice_uint8_t string__format(__Slice_uint8_t text, __Slice_uint8_t buf, __Slice_int32_t args) {
  uint32_t ti = 0;
  uint32_t bi = 0;
  while (((ti < text.size) && (bi < buf.size))) {
    uint8_t c = text.ptr[ti];
    if ((c == '{')) {
      (ti += 1);
      int32_t idx = 0;
      while ((((((((ti < text.size) && (text.ptr[ti] != '}')) && (text.ptr[ti] != 'i')) && (text.ptr[ti] != 'u')) && (text.ptr[ti] != 'f')) && (text.ptr[ti] != 'd')) && (text.ptr[ti] != 'b'))) {
        (idx = (((idx * 10) + ((int32_t)(text.ptr[ti]))) - 48));
        (ti += 1);
      }
      uint8_t spec = 'i';
      if (((ti < text.size) && (text.ptr[ti] != '}'))) {
        (spec = text.ptr[ti]);
        (ti += 1);
      }
      (ti += 1);
      if (((idx >= 0) && (idx < ((int32_t)(args.size))))) {
        __Slice_uint8_t sub = {(buf.ptr + bi), (buf.size - bi)};
        uint32_t written = 0;
        if ((spec == 'u')) {
          (written = string__format_u32(sub, ((uint32_t)(args.ptr[idx]))));
        } else if ((spec == 'f')) {
          (written = string___format_float(sub, __mp_bits_to_float(args.ptr[idx])));
        } else if ((spec == 'd')) {
          (written = string___format_fixed(sub, args.ptr[idx]));
        } else if ((spec == 'b')) {
          (written = string___format_bool(sub, args.ptr[idx]));
        } else {
          (written = string__format_i32(sub, args.ptr[idx]));
        }
        (bi += written);
      }
    } else {
      (buf.ptr[bi] = c);
      (bi += 1);
      (ti += 1);
    }
  }
  return (__Slice_uint8_t){buf.ptr, bi};
}

void app_main(void) { main__app_main(); }

