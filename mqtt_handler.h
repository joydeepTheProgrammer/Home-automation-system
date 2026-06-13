/* MQTT Handler - Pub/Sub for Home Assistant Integration */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*relay_cmd_cb_t)(uint8_t relay_id, bool state);

void mqtt_handler_init(void);
void mqtt_handler_register_callback(relay_cmd_cb_t cb);
void mqtt_handler_connect(void);
void mqtt_handler_disconnect(void);
bool mqtt_handler_connected(void);
void mqtt_handler_publish_relay_state(uint8_t relay_id, bool state);
void mqtt_handler_publish_all_states(void);
void mqtt_handler_publish_sensor(float temp, float humidity);
void mqtt_handler_publish_availability(bool online);
void mqtt_handler_loop(void);

#endif /* MQTT_HANDLER_H */
