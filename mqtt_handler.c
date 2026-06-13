/* MQTT Handler Implementation using ESP-IDF MQTT Client */

#include "mqtt_handler.h"
#include "config.h"
#include "gpio_manager.h"
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;
static relay_cmd_cb_t relay_cmd_cb = NULL;
static uint32_t last_reconnect_attempt = 0;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_RELAY_1, 0);
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_RELAY_2, 0);
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_RELAY_3, 0);
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_RELAY_4, 0);
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_ALL, 0);
        mqtt_handler_publish_availability(true);
        mqtt_handler_publish_all_states();
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_DATA: {
        char topic[64];
        char data[16];
        int tlen = event->topic_len;
        int dlen = event->data_len;
        if (tlen >= (int)sizeof(topic)) tlen = (int)sizeof(topic) - 1;
        if (dlen >= (int)sizeof(data)) dlen = (int)sizeof(data) - 1;
        memcpy(topic, event->topic, tlen);
        topic[tlen] = '\0';
        memcpy(data, event->data, dlen);
        data[dlen] = '\0';

        if (strcmp(topic, MQTT_TOPIC_ALL) == 0) {
            if (strcmp(data, PAYLOAD_ON) == 0) {
                for (uint8_t i = 0; i < NUM_RELAYS; i++) {
                    if (relay_cmd_cb) relay_cmd_cb(i, true);
                }
            } else if (strcmp(data, PAYLOAD_OFF) == 0) {
                for (uint8_t i = 0; i < NUM_RELAYS; i++) {
                    if (relay_cmd_cb) relay_cmd_cb(i, false);
                }
            }
            break;
        }

        int relay_id = -1;
        if (strcmp(topic, MQTT_TOPIC_RELAY_1) == 0) relay_id = 0;
        else if (strcmp(topic, MQTT_TOPIC_RELAY_2) == 0) relay_id = 1;
        else if (strcmp(topic, MQTT_TOPIC_RELAY_3) == 0) relay_id = 2;
        else if (strcmp(topic, MQTT_TOPIC_RELAY_4) == 0) relay_id = 3;
        if (relay_id < 0) break;

        bool state;
        if (strcmp(data, PAYLOAD_ON) == 0) state = true;
        else if (strcmp(data, PAYLOAD_OFF) == 0) state = false;
        else if (strcmp(data, PAYLOAD_TOGGLE) == 0) {
            state = !relay_get_state((relay_id_t)relay_id);
        } else break;

        if (relay_cmd_cb) relay_cmd_cb((uint8_t)relay_id, state);
        break;
    }
    default:
        break;
    }
}

void mqtt_handler_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .session.keepalive = MQTT_KEEPALIVE_SEC,
        .session.last_will.topic = MQTT_TOPIC_AVAILABILITY,
        .session.last_will.msg = PAYLOAD_STATUS_OFFLINE,
        .session.last_will.msg_len = strlen(PAYLOAD_STATUS_OFFLINE),
        .session.last_will.qos = 0,
        .session.last_will.retain = 1,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_handler_register_callback(relay_cmd_cb_t cb)
{
    relay_cmd_cb = cb;
}

void mqtt_handler_connect(void)
{
    if (client == NULL) return;
    if (mqtt_connected) return;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_reconnect_attempt < MQTT_RECONNECT_MS) return;
    last_reconnect_attempt = now;
    ESP_LOGI(TAG, "Starting MQTT connection to %s", MQTT_BROKER_URI);
    esp_mqtt_client_start(client);
}

void mqtt_handler_disconnect(void)
{
    if (client == NULL) return;
    mqtt_handler_publish_availability(false);
    esp_mqtt_client_stop(client);
    mqtt_connected = false;
}

bool mqtt_handler_connected(void)
{
    return mqtt_connected;
}

void mqtt_handler_publish_relay_state(uint8_t relay_id, bool state)
{
    if (!mqtt_connected || client == NULL) return;
    const char *topic = NULL;
    switch (relay_id) {
        case 0: topic = MQTT_TOPIC_RELAY_1; break;
        case 1: topic = MQTT_TOPIC_RELAY_2; break;
        case 2: topic = MQTT_TOPIC_RELAY_3; break;
        case 3: topic = MQTT_TOPIC_RELAY_4; break;
        default: return;
    }
    const char *payload = state ? PAYLOAD_ON : PAYLOAD_OFF;
    esp_mqtt_client_publish(client, topic, payload, strlen(payload), 0, 1);
}

void mqtt_handler_publish_all_states(void)
{
    uint8_t i;
    for (i = 0; i < NUM_RELAYS; i++) {
        mqtt_handler_publish_relay_state(i, relay_get_state((relay_id_t)i));
    }
}

void mqtt_handler_publish_sensor(float temp, float humidity)
{
    if (!mqtt_connected || client == NULL) return;
    char payload[128];
    snprintf(payload, sizeof(payload), "{"temperature":%.1f,"humidity":%.1f}", temp, humidity);
    esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, payload, strlen(payload), 0, 0);
}

void mqtt_handler_publish_availability(bool online)
{
    if (!mqtt_connected && !online) return;
    if (client == NULL) return;
    const char *payload = online ? PAYLOAD_STATUS_ONLINE : PAYLOAD_STATUS_OFFLINE;
    esp_mqtt_client_publish(client, MQTT_TOPIC_AVAILABILITY, payload, strlen(payload), 0, 1);
}

void mqtt_handler_loop(void)
{
    if (!mqtt_connected) {
        mqtt_handler_connect();
    }
}
