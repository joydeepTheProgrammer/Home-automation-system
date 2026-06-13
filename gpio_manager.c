/* GPIO Manager Implementation (ESP-IDF) */

#include "gpio_manager.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct {
    bool state;
    bool manual_override;
    uint32_t on_time_ms;
    uint32_t toggle_count;
} relay_status_t;

typedef struct {
    bool last_state;
    bool current_state;
    uint32_t last_debounce_ms;
    uint32_t press_start_ms;
} switch_status_t;

static relay_status_t relays[NUM_RELAYS];
static switch_status_t switches[NUM_SWITCHES];
static uint32_t led_blink_period[LED_MAX];
static QueueHandle_t switch_evt_queue = NULL;

static const uint8_t relay_pins[NUM_RELAYS] = {
    GPIO_RELAY_1, GPIO_RELAY_2, GPIO_RELAY_3, GPIO_RELAY_4
};
static const uint8_t switch_pins[NUM_SWITCHES] = {
    GPIO_SWITCH_1, GPIO_SWITCH_2, GPIO_SWITCH_3, GPIO_SWITCH_4
};
static const uint8_t led_pins[LED_MAX] = {
    GPIO_LED_1, GPIO_LED_2, GPIO_LED_3, GPIO_LED_4, GPIO_STATUS_LED
};

void gpio_manager_init(void)
{
    uint8_t i;
    uint64_t relay_mask = 0;
    uint64_t switch_mask = 0;
    uint64_t led_mask = 0;

    for (i = 0; i < NUM_RELAYS; i++) {
        relay_mask |= (1ULL << relay_pins[i]);
        relays[i].state = false;
        relays[i].manual_override = false;
        relays[i].on_time_ms = 0;
        relays[i].toggle_count = 0;
    }
    for (i = 0; i < NUM_SWITCHES; i++) {
        switch_mask |= (1ULL << switch_pins[i]);
        switches[i].last_state = true;
        switches[i].current_state = true;
        switches[i].last_debounce_ms = 0;
        switches[i].press_start_ms = 0;
    }
    for (i = 0; i < LED_MAX; i++) {
        led_mask |= (1ULL << led_pins[i]);
        led_blink_period[i] = 0;
    }
    led_mask |= (1ULL << GPIO_BUZZER);

    gpio_config_t io_out = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = relay_mask | led_mask,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_out);

    gpio_config_t io_in = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = switch_mask,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_in);

    for (i = 0; i < NUM_RELAYS; i++) {
        gpio_set_level(relay_pins[i], RELAY_OFF);
    }
    for (i = 0; i < LED_MAX; i++) {
        gpio_set_level(led_pins[i], 0);
    }
    gpio_set_level(GPIO_BUZZER, 0);

    switch_evt_queue = xQueueCreate(16, sizeof(switch_event_t));
}

QueueHandle_t gpio_manager_get_queue(void)
{
    return switch_evt_queue;
}

void relay_set(relay_id_t relay, bool state)
{
    if (relay >= RELAY_MAX) return;
    if (state && relay_get_active_count() >= MAX_CONCURRENT_ON && !relays[relay].state) {
        return;
    }
    gpio_set_level(relay_pins[relay], state ? RELAY_ON : RELAY_OFF);
    relays[relay].state = state;
    relays[relay].toggle_count++;
    if (state) {
        relays[relay].on_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
    led_set((led_id_t)(LED_1 + relay), state);
}

void relay_toggle(relay_id_t relay)
{
    if (relay >= RELAY_MAX) return;
    relay_set(relay, !relays[relay].state);
}

bool relay_get_state(relay_id_t relay)
{
    if (relay >= RELAY_MAX) return false;
    return relays[relay].state;
}

void relay_all_off(void)
{
    uint8_t i;
    for (i = 0; i < NUM_RELAYS; i++) {
        relay_set((relay_id_t)i, false);
    }
}

void relay_all_on(void)
{
    uint8_t i;
    for (i = 0; i < NUM_RELAYS; i++) {
        relay_set((relay_id_t)i, true);
    }
}

uint8_t relay_get_active_count(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < NUM_RELAYS; i++) {
        if (relays[i].state) count++;
    }
    return count;
}

void led_set(led_id_t led, bool state)
{
    if (led >= LED_MAX) return;
    gpio_set_level(led_pins[led], state ? 1 : 0);
    led_blink_period[led] = 0;
}

void led_toggle(led_id_t led)
{
    if (led >= LED_MAX) return;
    gpio_set_level(led_pins[led], !gpio_get_level(led_pins[led]));
}

void led_blink(led_id_t led, uint32_t period_ms)
{
    if (led >= LED_MAX) return;
    led_blink_period[led] = period_ms;
}

void led_task_handler(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint8_t i;
    for (i = 0; i < LED_MAX; i++) {
        if (led_blink_period[i] > 0) {
            bool on = ((now / led_blink_period[i]) % 2) != 0;
            gpio_set_level(led_pins[i], on);
        }
    }
}

void switch_scan(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint8_t i;
    for (i = 0; i < NUM_SWITCHES; i++) {
        bool reading = gpio_get_level(switch_pins[i]);
        if (reading != switches[i].last_state) {
            switches[i].last_debounce_ms = now;
        }
        if ((now - switches[i].last_debounce_ms) > DEBOUNCE_MS) {
            if (reading != switches[i].current_state) {
                switches[i].current_state = reading;
                if (reading == 0) {
                    switches[i].press_start_ms = now;
                } else {
                    uint32_t dur = now - switches[i].press_start_ms;
                    switch_event_t evt = {
                        .id = (switch_id_t)i,
                        .long_press = (dur >= LONG_PRESS_MS)
                    };
                    if (switch_evt_queue) {
                        xQueueSend(switch_evt_queue, &evt, 0);
                    }
                }
            }
        }
        switches[i].last_state = reading;
    }
}

void switch_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        switch_scan();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void buzzer_beep(uint32_t duration_ms)
{
    gpio_set_level(GPIO_BUZZER, 1);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(GPIO_BUZZER, 0);
}

float read_temperature(void)
{
    return 25.0f;
}

float read_humidity(void)
{
    return 50.0f;
}
