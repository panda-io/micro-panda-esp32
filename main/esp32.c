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

typedef void (*__Fn_void_void)(void);

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

typedef enum {
  PwmTimer_TIMER0 = 0,
  PwmTimer_TIMER1 = 1,
  PwmTimer_TIMER2 = 2,
  PwmTimer_TIMER3 = 3,
} PwmTimer;

void main__blink(void);
void main__fade(void);
void main__task_producer(void);
void main__task_consumer(void);
void main__app_main(void);
void digital__pin_mode(int32_t pin, PinMode mode);
static inline void digital__digital_write(int32_t pin, PinLevel value);
static inline PinLevel digital__digital_read(int32_t pin);
static int32_t pwm___find(int32_t pin);
static int32_t pwm___alloc(int32_t pin);
void pwm__pwm_timer(PwmTimer timer, int32_t freq);
void pwm__pwm_attach(int32_t pin, PwmTimer timer);
void pwm__pwm_start(int32_t pin, int32_t duty);
void pwm__pwm_stop(int32_t pin);

const int32_t main__LED_PIN = 2;
const int32_t main__FADE_STEP = 32;
const int32_t main__FADE_DELAY = 20;
const int32_t main__BLINK_MS = 500;
int32_t main__duty = 0;
int32_t main__dir = 1;
void* main__task_handler = NULL;
static int32_t pwm___pins[6];
const int32_t config__PWM_MAX_CHANNELS = 6;
const int32_t config__RTOS_MAX_TASKS = 10;

void main__blink(void) {
  digital__pin_mode(main__LED_PIN, PinMode_OUTPUT);
  int32_t args[2];
  while (true) {
    digital__digital_write(main__LED_PIN, PinLevel_HIGH);
    __mp_delay_ms(main__BLINK_MS);
    digital__digital_write(main__LED_PIN, PinLevel_LOW);
    __mp_delay_ms(main__BLINK_MS);
    (args[0] = __mp_time_ms());
    (args[1] = xTaskGetTickCount());
    info_args((__Slice_uint8_t){(uint8_t*)"current time: {0i} ms", sizeof("current time: {0i} ms") - 1}, args);
    info_args((__Slice_uint8_t){(uint8_t*)"current ticks: {1i}", sizeof("current ticks: {1i}") - 1}, args);
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
    info_args((__Slice_uint8_t){(uint8_t*)"produce job in cpu core: {0i}", sizeof("produce job in cpu core: {0i}") - 1}, args);
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
    info_args((__Slice_uint8_t){(uint8_t*)"executing task in cpu core: {0i}", sizeof("executing task in cpu core: {0i}") - 1}, args);
  }
}

void main__app_main(void) {
  info((__Slice_uint8_t){(uint8_t*)"micro-panda esp32 ready", sizeof("micro-panda esp32 ready") - 1});
  (main__task_handler = __mp_task_create_pinned(main__task_consumer, 1024, 1, 1));
  __mp_task_create(main__task_producer, 1024, 1);
  main__blink();
  __mp_task_exit();
}

void digital__pin_mode(int32_t pin, PinMode mode) {
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

static inline void digital__digital_write(int32_t pin, PinLevel value) {
  gpio_set_level(pin, ((int32_t)(value)));
}

static inline PinLevel digital__digital_read(int32_t pin) {
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

void app_main(void) { main__app_main(); }

