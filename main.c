/*
 * Home Automation Controller - ESP-IDF (Standard C)
 * Features: 4-Channel Relay, MQTT, WiFi, Manual Switches, Status LEDs
 * Build: idf.py build
 * Flash: idf.py -p /dev/ttyUSB0 flash monitor
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "gpio_manager.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"

static const char *TAG = "HA_MAIN";

static void system_task(void *pvParameters);
static void on_relay_command(uint8_t relay_id, bool state);

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Home Automation Controller (C)");
    ESP_LOGI(TAG, "Build date: %s", __DATE__);

    ESP_ERROR_CHECK(nvs_flash_init());

    gpio_manager_init();
    mqtt_handler_init();
    mqtt_handler_register_callback(on_relay_command);

    wifi_manager_init();
    wifi_manager_start();
    wifi_manager_wait_connected();

    mqtt_handler_connect();

    relay_all_off();
    led_blink(LED_STATUS, 500);

    xTaskCreate(system_task, "system_task", 4096, NULL, 5, NULL);
    xTaskCreate(switch_task, "switch_task", 2048, NULL, 5, NULL);
}

static void on_relay_command(uint8_t relay_id, bool state)
{
    if (relay_id >= NUM_RELAYS) return;
    relay_set((relay_id_t)relay_id, state);
    mqtt_handler_publish_relay_state(relay_id, state);
    ESP_LOGI(TAG, "Relay%d -> %s", relay_id + 1, state ? "ON" : "OFF");
}

static void system_task(void *pvParameters)
{
    (void)pvParameters;
    switch_event_t evt;
    uint32_t last_sensor_ms = 0;
    uint32_t last_watchdog_ms = 0;
    QueueHandle_t sw_queue = gpio_manager_get_queue();

    for (;;) {
        if (xQueueReceive(sw_queue, &evt, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (evt.long_press) {
                relay_all_off();
                mqtt_handler_publish_all_states();
                buzzer_beep(200);
                ESP_LOGI(TAG, "Long press -> all relays OFF");
            } else {
                relay_toggle(evt.id);
                bool st = relay_get_state(evt.id);
                mqtt_handler_publish_relay_state((uint8_t)evt.id, st);
                buzzer_beep(50);
                ESP_LOGI(TAG, "Switch%d toggle -> Relay%d %s",
                         evt.id + 1, evt.id + 1, st ? "ON" : "OFF");
            }
        }

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((now - last_sensor_ms) >= SENSOR_READ_INTERVAL_MS) {
            last_sensor_ms = now;
            float t = read_temperature();
            float h = read_humidity();
            mqtt_handler_publish_sensor(t, h);
            ESP_LOGI(TAG, "Sensor: T=%.1fC  H=%.1f%%", t, h);

            if (t > THERMAL_SHUTDOWN_C) {
                ESP_LOGE(TAG, "Thermal shutdown triggered! T=%.1fC", t);
                relay_all_off();
                mqtt_handler_publish_all_states();
            }
        }

        if ((now - last_watchdog_ms) >= 10000) {
            last_watchdog_ms = now;
            if (!wifi_manager_is_connected()) {
                ESP_LOGW(TAG, "WiFi lost, reconnecting...");
                wifi_manager_start();
            }
            if (!mqtt_handler_connected()) {
                ESP_LOGW(TAG, "MQTT lost, reconnecting...");
                mqtt_handler_connect();
            }
            if (relay_get_active_count() > MAX_CONCURRENT_ON) {
                ESP_LOGW(TAG, "Overload detected, shutting down relays");
                relay_all_off();
                mqtt_handler_publish_all_states();
            }
            mqtt_handler_publish_availability(true);
        }

        led_task_handler();
    }
}
