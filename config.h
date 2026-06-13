/* Home Automation System - ESP-IDF Configuration
 * Hardware: ESP32-WROOM-32
 * Features: 4-Channel Relay, MQTT, WiFi, Manual Switches, Status LEDs
 */

#ifndef CONFIG_H
#define CONFIG_H

/* WiFi Credentials */
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define WIFI_MAX_RETRIES        20
#define WIFI_RETRY_DELAY_MS     500

/* MQTT Broker Settings */
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
#define MQTT_CLIENT_ID          "esp32_home_auto"
#define MQTT_USERNAME           ""
#define MQTT_PASSWORD           ""
#define MQTT_KEEPALIVE_SEC      60
#define MQTT_RECONNECT_MS       5000

/* MQTT Topics */
#define MQTT_TOPIC_PREFIX       "home/livingroom"
#define MQTT_TOPIC_RELAY_1      MQTT_TOPIC_PREFIX "/relay1"
#define MQTT_TOPIC_RELAY_2      MQTT_TOPIC_PREFIX "/relay2"
#define MQTT_TOPIC_RELAY_3      MQTT_TOPIC_PREFIX "/relay3"
#define MQTT_TOPIC_RELAY_4      MQTT_TOPIC_PREFIX "/relay4"
#define MQTT_TOPIC_ALL          MQTT_TOPIC_PREFIX "/all"
#define MQTT_TOPIC_STATUS       MQTT_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_AVAILABILITY MQTT_TOPIC_PREFIX "/availability"

/* GPIO Pin Mapping (ESP32-WROOM-32 DevKit) */
#define GPIO_RELAY_1            23
#define GPIO_RELAY_2            22
#define GPIO_RELAY_3            21
#define GPIO_RELAY_4            19
#define GPIO_LED_1              18
#define GPIO_LED_2              5
#define GPIO_LED_3              17
#define GPIO_LED_4              16
#define GPIO_SWITCH_1           34
#define GPIO_SWITCH_2           35
#define GPIO_SWITCH_3           32
#define GPIO_SWITCH_4           33
#define GPIO_BUZZER             25
#define GPIO_DHT_SENSOR         26
#define GPIO_STATUS_LED         2

/* System Parameters */
#define NUM_RELAYS              4
#define NUM_SWITCHES            4
#define NUM_STATUS_LEDS         4
#define DEBOUNCE_MS             50
#define LONG_PRESS_MS           1000
#define SENSOR_READ_INTERVAL_MS 30000
#define UART_BAUD_RATE          115200

/* Relay Logic */
#define RELAY_ON                1
#define RELAY_OFF               0

/* MQTT Payload Values */
#define PAYLOAD_ON              "ON"
#define PAYLOAD_OFF             "OFF"
#define PAYLOAD_TOGGLE          "TOGGLE"
#define PAYLOAD_STATUS_ONLINE   "online"
#define PAYLOAD_STATUS_OFFLINE  "offline"

/* Safety Limits */
#define MAX_CONCURRENT_ON       3
#define THERMAL_SHUTDOWN_C      75.0f

#endif /* CONFIG_H */
