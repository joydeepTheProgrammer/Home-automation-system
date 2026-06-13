/* WiFi Manager - ESP-IDF Station Mode */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

void wifi_manager_init(void);
void wifi_manager_start(void);
void wifi_manager_wait_connected(void);
bool wifi_manager_is_connected(void);

#endif /* WIFI_MANAGER_H */
