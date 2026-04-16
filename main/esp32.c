#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  GpioMode_INPUT = 1,
  GpioMode_OUTPUT = 3,
  GpioMode_INPUT_PULLUP = 5,
  GpioMode_INPUT_PULLDOWN = 6,
} GpioMode;

typedef enum {
  GpioLevel_LOW = 0,
  GpioLevel_HIGH = 1,
} GpioLevel;

typedef enum {
  PwmTimer_TIMER0 = 0,
  PwmTimer_TIMER1 = 1,
  PwmTimer_TIMER2 = 2,
  PwmTimer_TIMER3 = 3,
} PwmTimer;

typedef struct { uint8_t* ptr; int32_t size; } __Slice_uint8_t;
typedef struct { int32_t* ptr; int32_t size; } __Slice_int32_t;

typedef void (*__Fn_void)(void);
typedef void (*__Fn_void_uint8_t)(uint8_t);

#define PWM_MAX_CHANNELS 8
#define I2C_MAX_BUSES 2
#define I2C_MAX_DEVICES 8
#define I2C_TIMEOUT_MS 1000
#define SPI_MAX_BUSES 2
#define SPI_MAX_DEVICES 6
#define SPI_DMA 1
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

static inline void    __mp_delay_ms(int32_t ms)     { vTaskDelay(pdMS_TO_TICKS(ms)); }
static inline int32_t __mp_time_ms(void)             { return (int32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS); }
static inline int32_t __mp_task_core_id(void)            { return (int32_t)xPortGetCoreID(); }

static inline void*   __mp_task_create(void (*fn)(void), int32_t stack, int32_t priority) {
    TaskHandle_t h = NULL;
    xTaskCreate((TaskFunction_t)(void*)fn, "", (uint32_t)stack, NULL, (UBaseType_t)priority, &h);
    return (void*)h;
}
static inline void*   __mp_task_create_pinned(void (*fn)(void), int32_t stack, int32_t priority, int32_t core) {
    TaskHandle_t h = NULL;
    xTaskCreatePinnedToCore((TaskFunction_t)(void*)fn, "", (uint32_t)stack, NULL, (UBaseType_t)priority, &h, (BaseType_t)core);
    return (void*)h;
}
static inline void    __mp_task_exit(void)           { vTaskDelete(NULL); }
static inline void    __mp_task_notify(void* handle) { xTaskNotify((TaskHandle_t)handle, 0, eNoAction); }
static inline int32_t __mp_task_wait(void)           { return (int32_t)ulTaskNotifyTake(pdTRUE, portMAX_DELAY); }
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t __mp_i2c_buses[I2C_MAX_BUSES];
static i2c_master_dev_handle_t __mp_i2c_devs[I2C_MAX_DEVICES];
static uint32_t                __mp_i2c_freq[I2C_MAX_BUSES];

// Initialize I2C bus. Returns bus index or -1 on error.
static inline int32_t __mp_i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
    if (bus < 0 || bus >= I2C_MAX_BUSES) return -1;
    if (__mp_i2c_buses[bus] != NULL) return bus; // already initialized
    i2c_master_bus_config_t cfg = {
        .i2c_port    = (i2c_port_num_t)bus,
        .sda_io_num  = (gpio_num_t)sda,
        .scl_io_num  = (gpio_num_t)scl,
        .clk_source  = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &__mp_i2c_buses[bus]);
    if (err != ESP_OK) return -1;
    __mp_i2c_freq[bus] = (uint32_t)freq_hz;
    return bus;
}

// Open a device on the bus by 7-bit address. Returns device slot or -1 on error.
static inline int32_t __mp_i2c_open(int32_t bus, int32_t addr) {
    if (bus < 0 || bus >= I2C_MAX_BUSES || __mp_i2c_buses[bus] == NULL) return -1;
    for (int i = 0; i < I2C_MAX_DEVICES; i++) {
        if (__mp_i2c_devs[i] == NULL) {
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address  = (uint16_t)addr,
                .scl_speed_hz    = __mp_i2c_freq[bus],
            };
            esp_err_t err = i2c_master_bus_add_device(__mp_i2c_buses[bus], &dev_cfg, &__mp_i2c_devs[i]);
            return err == ESP_OK ? i : -1;
        }
    }
    return -1; // no free slot
}

static inline void __mp_i2c_close(int32_t dev) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return;
    i2c_master_bus_rm_device(__mp_i2c_devs[dev]);
    __mp_i2c_devs[dev] = NULL;
}

static inline int32_t __mp_i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit(__mp_i2c_devs[dev], data.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_receive(__mp_i2c_devs[dev], buf.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_write_read(int32_t dev,
                                           __Slice_uint8_t tx, int32_t tx_len,
                                           __Slice_uint8_t rx, int32_t rx_len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit_receive(
        __mp_i2c_devs[dev],
        tx.ptr, (size_t)tx_len,
        rx.ptr, (size_t)rx_len,
        I2C_TIMEOUT_MS);
}
#include <stdio.h>
static inline int32_t __mp_float_to_bits(float f) { int32_t v; __builtin_memcpy(&v, &f, 4); return v; }
static inline float __mp_bits_to_float(int32_t v) { float f; __builtin_memcpy(&f, &v, 4); return f; }

void main__blink(void);
void main__fade(void);
void main__task_producer(void);
void main__task_consumer(void);
void main__main(void);
void gpio__gpio_mode(int32_t pin, GpioMode mode);
static inline void gpio__gpio_write(int32_t pin, GpioLevel value);
static inline GpioLevel gpio__gpio_read(int32_t pin);
static int32_t pwm___find(int32_t pin);
static int32_t pwm___alloc(int32_t pin);
void pwm__pwm_timer(PwmTimer timer, int32_t freq);
void pwm__pwm_attach(int32_t pin, PwmTimer timer);
void pwm__pwm_start(int32_t pin, int32_t duty);
void pwm__pwm_stop(int32_t pin);
static inline void log___esc(__Slice_uint8_t seq);
void log__info(__Slice_uint8_t message);
void log__warn(__Slice_uint8_t message);
void log__error(__Slice_uint8_t message);
void log__info_args(__Slice_uint8_t message, __Slice_int32_t args);
void log__warn_args(__Slice_uint8_t message, __Slice_int32_t args);
void log__error_args(__Slice_uint8_t message, __Slice_int32_t args);
int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz);
int32_t i2c__i2c_open(int32_t bus, int32_t addr);
void i2c__i2c_close(int32_t dev);
int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len);
int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len);
int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len);
static void console___write_byte_default(uint8_t b);
void console__init(__Fn_void_uint8_t write_byte);
static inline void console__write_byte(uint8_t b);
void console__print(__Slice_uint8_t string);
void console__print_args(__Slice_uint8_t string, __Slice_int32_t args);
void console__write_string(__Slice_uint8_t string);
void console__write_bool(bool value);
void console__write_u64(uint64_t value);
void console__write_u32(uint32_t value);
void console__write_u16(uint16_t value);
void console__write_u8(uint8_t value);
void console__write_i64(int64_t value);
void console__write_i32(int32_t value);
void console__write_i16(int16_t value);
void console__write_i8(int8_t value);
void console__write_int(int32_t value);
void console__write_float(float value);
void console__write_q16(int32_t value);
void console__write_fixed(int32_t value);
bool string__equals(__Slice_uint8_t a, __Slice_uint8_t b);
bool string__starts_with(__Slice_uint8_t string, __Slice_uint8_t prefix);
bool string__ends_with(__Slice_uint8_t string, __Slice_uint8_t suffix);
int32_t string__index_of(__Slice_uint8_t string, uint8_t character);
__Slice_uint8_t string__sub(__Slice_uint8_t string, int32_t start, int32_t len);
__Slice_uint8_t string__trim_start(__Slice_uint8_t string);
__Slice_uint8_t string__trim_end(__Slice_uint8_t string);
__Slice_uint8_t string__trim(__Slice_uint8_t string);
__Slice_uint8_t string__token(__Slice_uint8_t string, int32_t start, uint8_t delim);
int32_t string__skip(__Slice_uint8_t string, int32_t start, uint8_t delim);
uint32_t string__parse_u32(__Slice_uint8_t string);
int32_t string__parse_i32(__Slice_uint8_t string);
int32_t string__parse_int(__Slice_uint8_t string);
int32_t string__format_u32(__Slice_uint8_t buf, uint32_t value);
int32_t string__format_i32(__Slice_uint8_t buf, int32_t value);
int32_t string__format_int(__Slice_uint8_t buf, int32_t value);
static int32_t string___format_float(__Slice_uint8_t buf, float value);
static int32_t string___format_fixed(__Slice_uint8_t buf, int32_t value);
static int32_t string___format_bool(__Slice_uint8_t buf, int32_t value);
__Slice_uint8_t string__format(__Slice_uint8_t text, __Slice_uint8_t buf, __Slice_int32_t args);

const int32_t main__LED_PIN = 2;
const int32_t main__FADE_STEP = 32;
const int32_t main__FADE_DELAY = 20;
const int32_t main__BLINK_MS = 500;
const int32_t main__OLED_SDA = 21;
const int32_t main__OLED_SCL = 22;
const int32_t main__OLED_ADDR = 0x3C;
int32_t main__duty = 0;
int32_t main__dir = 1;
void* main__task_handler = NULL;
static int32_t pwm___pins[1];
static __Fn_void_uint8_t console___write_byte = console___write_byte_default;

void main__blink(void) {
  gpio__gpio_mode(main__LED_PIN, GpioMode_OUTPUT);
  int32_t args[2];
  while (true) {
    gpio__gpio_write(main__LED_PIN, GpioLevel_HIGH);
    __mp_delay_ms(main__BLINK_MS);
    gpio__gpio_write(main__LED_PIN, GpioLevel_LOW);
    __mp_delay_ms(main__BLINK_MS);
    (args[0] = __mp_time_ms());
    (args[1] = xTaskGetTickCount());
    log__info_args((__Slice_uint8_t){(uint8_t*)"current time: {0i} ms", sizeof("current time: {0i} ms") - 1}, (__Slice_int32_t){args, 2});
    log__info_args((__Slice_uint8_t){(uint8_t*)"current ticks: {1i}", sizeof("current ticks: {1i}") - 1}, (__Slice_int32_t){args, 2});
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

void main__task_producer(void) {
  int32_t count = 10;
  int32_t args[1];
  (args[0] = __mp_task_core_id());
  while ((count > 0)) {
    log__info_args((__Slice_uint8_t){(uint8_t*)"produce job in cpu core: {0i}", sizeof("produce job in cpu core: {0i}") - 1}, (__Slice_int32_t){args, 1});
    __mp_task_notify(main__task_handler);
    (count -= 1);
    __mp_delay_ms(512);
  }
  __mp_task_exit();
}

void main__task_consumer(void) {
  int32_t args[1];
  (args[0] = __mp_task_core_id());
  while (true) {
    __mp_task_wait();
    log__info_args((__Slice_uint8_t){(uint8_t*)"executing task in cpu core: {0i}", sizeof("executing task in cpu core: {0i}") - 1}, (__Slice_int32_t){args, 1});
  }
}

void main__main(void) {
  log__info((__Slice_uint8_t){(uint8_t*)"micro-panda esp32 ready", sizeof("micro-panda esp32 ready") - 1});
  (main__task_handler = __mp_task_create_pinned(main__task_consumer, 1024, 1, 1));
  __mp_task_create(main__task_producer, 1024, 1);
  main__blink();
  __mp_task_exit();
}

void gpio__gpio_mode(int32_t pin, GpioMode mode) {
  gpio_reset_pin(pin);
  if ((mode == GpioMode_INPUT_PULLUP)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 0);
  } else if ((mode == GpioMode_INPUT_PULLDOWN)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 1);
  } else {
    gpio_set_direction(pin, ((int32_t)(mode)));
  }
}

static inline void gpio__gpio_write(int32_t pin, GpioLevel value) {
  gpio_set_level(pin, ((int32_t)(value)));
}

static inline GpioLevel gpio__gpio_read(int32_t pin) {
  return ((GpioLevel)(gpio_get_level(pin)));
}

static int32_t pwm___find(int32_t pin) {
  int32_t i = 0;
  while ((i < PWM_MAX_CHANNELS)) {
    if ((pwm___pins[i] == (pin + 1))) {
      return i;
    }
    (i += 1);
  }
  return (-1);
}

static int32_t pwm___alloc(int32_t pin) {
  int32_t i = 0;
  while ((i < PWM_MAX_CHANNELS)) {
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
  console__write_string(seq);
}

void log__info(__Slice_uint8_t message) {
  log___esc((__Slice_uint8_t){(uint8_t*)"32m", sizeof("32m") - 1});
  console__write_string((__Slice_uint8_t){(uint8_t*)"[INFO] ", sizeof("[INFO] ") - 1});
  console__write_string(message);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__write_byte(10);
}

void log__warn(__Slice_uint8_t message) {
  log___esc((__Slice_uint8_t){(uint8_t*)"33m", sizeof("33m") - 1});
  console__write_string((__Slice_uint8_t){(uint8_t*)"[WARN] ", sizeof("[WARN] ") - 1});
  console__write_string(message);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__write_byte(10);
}

void log__error(__Slice_uint8_t message) {
  log___esc((__Slice_uint8_t){(uint8_t*)"31m", sizeof("31m") - 1});
  console__write_string((__Slice_uint8_t){(uint8_t*)"[ERROR] ", sizeof("[ERROR] ") - 1});
  console__write_string(message);
  log___esc((__Slice_uint8_t){(uint8_t*)"0m", sizeof("0m") - 1});
  console__write_byte(10);
}

void log__info_args(__Slice_uint8_t message, __Slice_int32_t args) {
  uint8_t buf[128];
  log__info(string__format(message, (__Slice_uint8_t){buf, 128}, args));
}

void log__warn_args(__Slice_uint8_t message, __Slice_int32_t args) {
  uint8_t buf[128];
  log__warn(string__format(message, (__Slice_uint8_t){buf, 128}, args));
}

void log__error_args(__Slice_uint8_t message, __Slice_int32_t args) {
  uint8_t buf[128];
  log__error(string__format(message, (__Slice_uint8_t){buf, 128}, args));
}

int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
  return __mp_i2c_init(bus, sda, scl, freq_hz);
}

int32_t i2c__i2c_open(int32_t bus, int32_t addr) {
  return __mp_i2c_open(bus, addr);
}

void i2c__i2c_close(int32_t dev) {
  __mp_i2c_close(dev);
}

int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
  return __mp_i2c_write(dev, data, len);
}

int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
  return __mp_i2c_read(dev, buf, len);
}

int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len) {
  return __mp_i2c_write_read(dev, tx, tx_len, rx, rx_len);
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

void console__print(__Slice_uint8_t string) {
  console__write_string(string);
  console__write_byte(10);
}

void console__print_args(__Slice_uint8_t string, __Slice_int32_t args) {
  uint8_t buf[128];
  console__print(string__format(string, (__Slice_uint8_t){buf, 128}, args));
}

void console__write_string(__Slice_uint8_t string) {
  for (int32_t i = 0; i < string.size; i++) {
    console__write_byte(string.ptr[i]);
  }
}

void console__write_bool(bool value) {
  if (value) {
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

void console__write_u64(uint64_t value) {
  uint8_t buf[20];
  int32_t i = 19;
  if ((value == 0)) {
    console__write_byte('0');
    return;
  }
  while ((value > 0)) {
    (buf[i] = ((uint8_t)(((value % 10) + 48))));
    (value = (value / 10));
    (i -= 1);
  }
  for (int32_t j = (i + 1); j < 20; j++) {
    console__write_byte(buf[j]);
  }
}

void console__write_u32(uint32_t value) {
  console__write_u64(((uint64_t)(value)));
}

void console__write_u16(uint16_t value) {
  console__write_u64(((uint64_t)(value)));
}

void console__write_u8(uint8_t value) {
  console__write_u64(((uint64_t)(value)));
}

void console__write_i64(int64_t value) {
  if ((value < 0)) {
    console__write_byte('-');
    console__write_u64(((uint64_t)((((int64_t)(0)) - value))));
  } else {
    console__write_u64(((uint64_t)(value)));
  }
}

void console__write_i32(int32_t value) {
  console__write_i64(((int64_t)(value)));
}

void console__write_i16(int16_t value) {
  console__write_i64(((int64_t)(value)));
}

void console__write_i8(int8_t value) {
  console__write_i64(((int64_t)(value)));
}

void console__write_int(int32_t value) {
  console__write_i64(((int64_t)(value)));
}

void console__write_float(float value) {
  float abs = value;
  if ((value < 0.0f)) {
    console__write_byte('-');
    (abs = (0.0f - value));
  }
  int32_t int_part = ((int32_t)(abs));
  console__write_int(int_part);
  console__write_byte('.');
  float frac = (abs - ((float)(int_part)));
  for (int32_t i = 0; i < 4; i++) {
    (frac = (frac * 10.0f));
    int32_t digit = ((int32_t)(frac));
    console__write_byte(((uint8_t)((digit + 48))));
    (frac = (frac - ((float)(digit))));
  }
}

void console__write_q16(int32_t value) {
  uint32_t abs = 0;
  if ((value < 0)) {
    console__write_byte('-');
    (abs = ((uint32_t)((0 - value))));
  } else {
    (abs = ((uint32_t)(value)));
  }
  console__write_u32((abs >> 16));
  console__write_byte('.');
  uint32_t frac = (abs & 0xFFFF);
  for (int32_t i = 0; i < 4; i++) {
    (frac *= 10);
    console__write_byte(((uint8_t)(((frac >> 16) + 48))));
    (frac = (frac & 0xFFFF));
  }
}

void console__write_fixed(int32_t value) {
  console__write_q16(((int32_t)(value)));
}

bool string__equals(__Slice_uint8_t a, __Slice_uint8_t b) {
  if ((a.size != b.size)) {
    return false;
  }
  for (int32_t i = 0; i < a.size; i++) {
    if ((a.ptr[i] != b.ptr[i])) {
      return false;
    }
  }
  return true;
}

bool string__starts_with(__Slice_uint8_t string, __Slice_uint8_t prefix) {
  if ((prefix.size > string.size)) {
    return false;
  }
  for (int32_t i = 0; i < prefix.size; i++) {
    if ((string.ptr[i] != prefix.ptr[i])) {
      return false;
    }
  }
  return true;
}

bool string__ends_with(__Slice_uint8_t string, __Slice_uint8_t suffix) {
  if ((suffix.size > string.size)) {
    return false;
  }
  int32_t offset = (string.size - suffix.size);
  for (int32_t i = 0; i < suffix.size; i++) {
    if ((string.ptr[(offset + i)] != suffix.ptr[i])) {
      return false;
    }
  }
  return true;
}

int32_t string__index_of(__Slice_uint8_t string, uint8_t character) {
  for (int32_t i = 0; i < string.size; i++) {
    if ((string.ptr[i] == character)) {
      return i;
    }
  }
  return (-1);
}

__Slice_uint8_t string__sub(__Slice_uint8_t string, int32_t start, int32_t len) {
  return (__Slice_uint8_t){(string.ptr + start), len};
}

__Slice_uint8_t string__trim_start(__Slice_uint8_t string) {
  for (int32_t i = 0; i < string.size; i++) {
    uint8_t c = string.ptr[i];
    if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
      return (__Slice_uint8_t){(string.ptr + i), (string.size - i)};
    }
  }
  return (__Slice_uint8_t){string.ptr, 0};
}

__Slice_uint8_t string__trim_end(__Slice_uint8_t string) {
  if ((string.size == 0)) {
    return string;
  }
  int32_t i = (string.size - 1);
  while ((i > 0)) {
    uint8_t c = string.ptr[i];
    if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
      return (__Slice_uint8_t){string.ptr, (i + 1)};
    }
    (i -= 1);
  }
  uint8_t c = string.ptr[0];
  if (((((c != 32) && (c != 9)) && (c != 10)) && (c != 13))) {
    return (__Slice_uint8_t){string.ptr, 1};
  }
  return (__Slice_uint8_t){string.ptr, 0};
}

__Slice_uint8_t string__trim(__Slice_uint8_t string) {
  return string__trim_end(string__trim_start(string));
}

__Slice_uint8_t string__token(__Slice_uint8_t string, int32_t start, uint8_t delim) {
  for (int32_t i = start; i < string.size; i++) {
    if ((string.ptr[i] == delim)) {
      return (__Slice_uint8_t){(string.ptr + start), (i - start)};
    }
  }
  return (__Slice_uint8_t){(string.ptr + start), (string.size - start)};
}

int32_t string__skip(__Slice_uint8_t string, int32_t start, uint8_t delim) {
  for (int32_t i = start; i < string.size; i++) {
    if ((string.ptr[i] != delim)) {
      return i;
    }
  }
  return string.size;
}

uint32_t string__parse_u32(__Slice_uint8_t string) {
  uint32_t result = 0;
  for (int32_t i = 0; i < string.size; i++) {
    uint8_t c = string.ptr[i];
    if (((c < 48) || (c > 57))) {
      return result;
    }
    (result = ((result * 10) + ((uint32_t)((c - 48)))));
  }
  return result;
}

int32_t string__parse_i32(__Slice_uint8_t string) {
  if ((string.size == 0)) {
    return 0;
  }
  if ((string.ptr[0] == 45)) {
    uint32_t abs = string__parse_u32((__Slice_uint8_t){(string.ptr + 1), (string.size - 1)});
    return ((int32_t)((-abs)));
  }
  return ((int32_t)(string__parse_u32(string)));
}

int32_t string__parse_int(__Slice_uint8_t string) {
  return ((int32_t)(string__parse_i32(string)));
}

int32_t string__format_u32(__Slice_uint8_t buf, uint32_t value) {
  if ((value == 0)) {
    (buf.ptr[0] = 48);
    return 1;
  }
  uint8_t tmp[10];
  int32_t len = 0;
  uint32_t n = value;
  while ((n > 0)) {
    (tmp[len] = ((uint8_t)((48 + (n % 10)))));
    (n = (n / 10));
    (len += 1);
  }
  for (int32_t i = 0; i < len; i++) {
    (buf.ptr[i] = tmp[((len - 1) - i)]);
  }
  return len;
}

int32_t string__format_i32(__Slice_uint8_t buf, int32_t value) {
  if ((value < 0)) {
    (buf.ptr[0] = 45);
    int32_t written = string__format_u32((__Slice_uint8_t){(buf.ptr + 1), (buf.size - 1)}, ((uint32_t)((-value))));
    return (written + 1);
  }
  return string__format_u32(buf, ((uint32_t)(value)));
}

int32_t string__format_int(__Slice_uint8_t buf, int32_t value) {
  return string__format_i32(buf, ((int32_t)(value)));
}

static int32_t string___format_float(__Slice_uint8_t buf, float value) {
  int32_t bi = 0;
  float abs = value;
  if ((value < 0.0f)) {
    (buf.ptr[bi] = 45);
    (bi += 1);
    (abs = (0.0f - value));
  }
  int32_t int_part = ((int32_t)(abs));
  (bi += string__format_int((__Slice_uint8_t){(buf.ptr + bi), (buf.size - bi)}, int_part));
  (buf.ptr[bi] = 46);
  (bi += 1);
  float frac = (abs - ((float)(int_part)));
  for (int32_t d = 0; d < 4; d++) {
    (frac = (frac * 10.0f));
    int32_t digit = ((int32_t)(frac));
    (buf.ptr[bi] = ((uint8_t)((digit + 48))));
    (bi += 1);
    (frac = (frac - ((float)(digit))));
  }
  return bi;
}

static int32_t string___format_fixed(__Slice_uint8_t buf, int32_t value) {
  int32_t bi = 0;
  uint32_t abs = 0;
  if ((value < 0)) {
    (buf.ptr[bi] = 45);
    (bi += 1);
    (abs = ((uint32_t)((0 - value))));
  } else {
    (abs = ((uint32_t)(value)));
  }
  (bi += string__format_u32((__Slice_uint8_t){(buf.ptr + bi), (buf.size - bi)}, (abs >> 16)));
  (buf.ptr[bi] = 46);
  (bi += 1);
  uint32_t frac = (abs & 0xFFFF);
  for (int32_t d = 0; d < 4; d++) {
    (frac *= 10);
    (buf.ptr[bi] = ((uint8_t)(((frac >> 16) + 48))));
    (bi += 1);
    (frac = (frac & 0xFFFF));
  }
  return bi;
}

static int32_t string___format_bool(__Slice_uint8_t buf, int32_t value) {
  if ((value != 0)) {
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
  int32_t ti = 0;
  int32_t bi = 0;
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
      if (((idx >= 0) && (idx < args.size))) {
        __Slice_uint8_t sub = (__Slice_uint8_t){(buf.ptr + bi), (buf.size - bi)};
        int32_t written = 0;
        if ((spec == 'u')) {
          (written = string__format_u32(sub, ((uint32_t)(args.ptr[idx]))));
        } else if ((spec == 'f')) {
          (written = string___format_float(sub, __mp_bits_to_float(args.ptr[idx])));
        } else if ((spec == 'd')) {
          (written = string___format_fixed(sub, args.ptr[idx]));
        } else if ((spec == 'b')) {
          (written = string___format_bool(sub, args.ptr[idx]));
        } else {
          (written = string__format_i32(sub, ((int32_t)(args.ptr[idx]))));
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

void app_main(void) { main__main(); }

