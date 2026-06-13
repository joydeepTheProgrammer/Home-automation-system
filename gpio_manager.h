/* GPIO Manager - Relay, LED, Switch, and Sensor Control */

#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    RELAY_1 = 0,
    RELAY_2,
    RELAY_3,
    RELAY_4,
    RELAY_MAX
} relay_id_t;

typedef enum {
    SWITCH_1 = 0,
    SWITCH_2,
    SWITCH_3,
    SWITCH_4,
    SWITCH_MAX
} switch_id_t;

typedef enum {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_4,
    LED_STATUS,
    LED_MAX
} led_id_t;

typedef struct {
    switch_id_t id;
    bool long_press;
} switch_event_t;

void gpio_manager_init(void);
QueueHandle_t gpio_manager_get_queue(void);

void relay_set(relay_id_t relay, bool state);
void relay_toggle(relay_id_t relay);
bool relay_get_state(relay_id_t relay);
void relay_all_off(void);
void relay_all_on(void);
uint8_t relay_get_active_count(void);

void led_set(led_id_t led, bool state);
void led_toggle(led_id_t led);
void led_blink(led_id_t led, uint32_t period_ms);
void led_task_handler(void);

void switch_scan(void);
void switch_task(void *pvParameters);
void buzzer_beep(uint32_t duration_ms);

float read_temperature(void);
float read_humidity(void);

#endif /* GPIO_MANAGER_H */
