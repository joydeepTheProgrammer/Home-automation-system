# ESP32 Home Automation Controller

A 4-channel WiFi-enabled relay board with physical wall switch support, built on the ESP32-WROOM-32 using ESP-IDF and MQTT.

![Schematic](output/schematic_diagram.png)

---

## Table of Contents
- [Features](#features)
- [Hardware](#hardware)
- [Pinout](#pinout)
- [Software](#software)
- [Build & Flash](#build--flash)
- [MQTT Topics](#mqtt-topics)
- [How It Works](#how-it-works)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Features

- **4 relay channels** (GPIO23, GPIO22, GPIO21, GPIO19)
- **4 physical wall switches** with software debouncing (GPIO34, GPIO35, GPIO32, GPIO33)
- **4 status LEDs** to show relay state (GPIO18, GPIO5, GPIO17, GPIO16)
- **WiFi auto-reconnect** with retry logic
- **MQTT integration** for remote control and monitoring
- **Sensor publishing** (temperature & humidity) every 30 seconds
- **Overload protection**: max 3 relays ON simultaneously
- **Long-press all-off**: hold any switch > 1 second to turn all relays off
- **Last Will & Testament**: broker auto-detects device disconnect
- **Home Assistant compatible** topic structure

---

## Hardware

### Components

| Component | Value | Package | Qty | Purpose |
|-----------|-------|---------|-----|---------|
| U1 | ESP32-WROOM-32 | SMD Module | 1 | MCU |
| C1 | 10uF | 1206 | 1 | Power decoupling |
| C2-C10 | 100nF | 0805 | 9 | Decoupling |
| R1-R4 | 10kΩ | 0805 | 4 | Switch pull-ups |
| R5-R8 | 1kΩ | 0805 | 4 | Transistor base resistors |
| R9-R12 | 330Ω | 0805 | 4 | LED current limit |
| R13-R16 | 1kΩ | 0805 | 4 | Relay base resistors |
| D1-D4 | LED (Status) | 0805 | 4 | Relay status indicators |
| D5-D8 | 1N4148 | SOD-123 | 4 | Flyback diodes |
| Q1-Q4 | S8050 | SOT-23 | 4 | NPN relay driver |
| RL1-RL4 | 5V Relay | THT | 4 | Load switching |
| J1 | 1x2 Pin Header | 2.54mm | 1 | Power input (5V/GND) |
| J6 | 1x4 Pin Header | 2.54mm | 1 | Switch inputs |
| J7 | 1x4 Pin Header | 2.54mm | 1 | UART programming |
| J2-J5 | 1x2 Pin Header | 2.54mm | 4 | Relay outputs |

### Power
- Input: **5V DC** via J1 (Pin 1 = +5V, Pin 2 = GND)
- ESP32 runs on 3.3V (internal LDO from 5V)
- Relays powered from 5V rail
- Decoupling: 10uF + 100nF near ESP32

---

## Pinout

### ESP32-WROOM-32 Pin Assignments

| Pin # | GPIO | Function | Direction | Notes |
|-------|------|----------|-----------|-------|
| 37 | IO23 | Relay 1 | Output | Through 1kΩ to Q1 base |
| 36 | IO22 | Relay 2 | Output | Through 1kΩ to Q2 base |
| 33 | IO21 | Relay 3 | Output | Through 1kΩ to Q3 base |
| 31 | IO19 | Relay 4 | Output | Through 1kΩ to Q4 base |
| 5 | IO34 | Switch 1 | Input | 10kΩ pull-up to 3.3V |
| 6 | IO35 | Switch 2 | Input | 10kΩ pull-up to 3.3V |
| 7 | IO32 | Switch 3 | Input | 10kΩ pull-up to 3.3V |
| 8 | IO33 | Switch 4 | Input | 10kΩ pull-up to 3.3V |
| 30 | IO18 | LED 1 | Output | Through 330Ω to GND |
| 29 | IO5 | LED 2 | Output | Through 330Ω to GND |
| 28 | IO17 | LED 3 | Output | Through 330Ω to GND |
| 27 | IO16 | LED 4 | Output | Through 330Ω to GND |
| 24 | IO2 | Status LED | Output | Onboard LED on devkit |
| 11 | IO25 | Buzzer | Output | Active high beeper |
| 10 | IO26 | DHT Sensor | Input | Temperature / humidity |
| 35 | TXD0 | UART TX | Output | Programming / debug |
| 34 | RXD0 | UART RX | Input | Programming / debug |
| 38 | 3V3 | Power | — | Output from regulator |
| 1 | EN | Enable | Input | Pull-up for normal boot |
| 13 | GND | Ground | — | Common ground |

---

## Software

### Architecture

```
app_main()
├── nvs_flash_init()
├── gpio_manager_init()
├── mqtt_handler_init()
├── wifi_manager_init()
├── wifi_manager_start()
├── wifi_manager_wait_connected()
├── mqtt_handler_connect()
├── relay_all_off()
├── xTaskCreate(system_task)
└── xTaskCreate(switch_task)

switch_task (10ms loop)
└── switch_scan()
    └── xQueueSend(switch_evt_queue)

system_task (event loop)
├── xQueueReceive(switch_evt_queue)
│   ├── relay_toggle() OR relay_all_off()
│   └── mqtt_publish_relay_state()
├── read_sensor() (every 30s)
│   └── mqtt_publish_sensor()
└── watchdog_check() (every 10s)
    ├── wifi reconnect if needed
    └── mqtt reconnect if needed

mqtt_event_handler (ISR callback)
├── MQTT_EVENT_CONNECTED → subscribe topics
└── MQTT_EVENT_DATA → parse command → relay_set()
```

### Files

| File | Purpose |
|------|---------|
| `main/main.c` | Application entry point and system task |
| `main/config.h` | Pin definitions, WiFi/MQTT config, thresholds |
| `main/gpio_manager.h/c` | GPIO abstraction: relay, switch, LED, buzzer, sensor |
| `main/wifi_manager.h/c` | WiFi station client with auto-reconnect |
| `main/mqtt_handler.h/c` | MQTT client with command parsing and publishing |
| `CMakeLists.txt` | ESP-IDF project build file |

---

## Build & Flash

### Prerequisites

- [ESP-IDF v5.0+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- USB-to-UART cable or ESP32 devkit USB
- MQTT broker (Mosquitto, Home Assistant, or HiveMQ)

### Configuration

Edit `main/config.h` before building:

```c
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"
```

Optional: change the topic prefix
```c
#define MQTT_TOPIC_PREFIX "home/livingroom"
```

### Build Commands

```bash
cd home_automation_c
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your actual serial port (Windows: `COM3`, macOS: `/dev/cu.usbserial-*`).

---

## MQTT Topics

### Topics the ESP32 Publishes To

| Topic | Payload | Retained | When |
|-------|---------|----------|------|
| `home/livingroom/relay1` | `ON` or `OFF` | Yes | Relay 1 changes |
| `home/livingroom/relay2` | `ON` or `OFF` | Yes | Relay 2 changes |
| `home/livingroom/relay3` | `ON` or `OFF` | Yes | Relay 3 changes |
| `home/livingroom/relay4` | `ON` or `OFF` | Yes | Relay 4 changes |
| `home/livingroom/status` | `{"temperature":25.0,"humidity":50.0}` | No | Every 30 seconds |
| `home/livingroom/availability` | `online` | Yes | On connect |
| `home/livingroom/availability` | `offline` | Yes | On disconnect (LWT) |

### Topics the ESP32 Subscribes To

| Topic | Payload | Action |
|-------|---------|--------|
| `home/livingroom/relay1` | `ON` / `OFF` / `TOGGLE` | Control relay 1 |
| `home/livingroom/relay2` | `ON` / `OFF` / `TOGGLE` | Control relay 2 |
| `home/livingroom/relay3` | `ON` / `OFF` / `TOGGLE` | Control relay 3 |
| `home/livingroom/relay4` | `ON` / `OFF` / `TOGGLE` | Control relay 4 |
| `home/livingroom/all` | `ON` / `OFF` | Group control |

### Home Assistant Configuration

```yaml
mqtt:
  switch:
    - name: "Living Room Relay 1"
      state_topic: "home/livingroom/relay1"
      command_topic: "home/livingroom/relay1"
      payload_on: "ON"
      payload_off: "OFF"
      state_on: "ON"
      state_off: "OFF"
      availability_topic: "home/livingroom/availability"
      payload_available: "online"
      payload_not_available: "offline"
```

---

## How It Works

### Physical Switch Control

1. Press wall switch → GPIO pin pulled to **GND** (LOW)
2. `switch_scan()` detects change after 50ms debounce
3. Measures press duration:
   - **< 1 second**: sends toggle event → relay toggles → MQTT publishes new state
   - **> 1 second**: sends long-press event → **all relays OFF** → MQTT publishes all states
4. Status LED matches relay state instantly

### Remote Control (MQTT)

1. Send `ON` to `home/livingroom/relay1`
2. ESP32 receives message in `mqtt_event_handler()`
3. Calls registered `relay_cmd_cb()` → `relay_set(RELAY_1, true)`
4. Relay coil energizes, LED lights up
5. ESP32 publishes `ON` back to confirm

### Safety Features

| Feature | Behavior |
|---------|----------|
| Max concurrent | Only 3 relays can be ON at once; 4th request is blocked |
| Thermal shutdown | Auto all-off if internal temp > 75°C |
| Auto reconnect | WiFi and MQTT reconnect automatically if lost |
| Buzzer feedback | Short beep on toggle, long beep on all-off |

---

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Relays not clicking | Wrong GPIO pin mapping | Check transistor base wiring matches `config.h` |
| Switch does nothing | Missing pull-up resistor | Add 10kΩ between GPIO and 3.3V |
| MQTT not connecting | Wrong broker IP or credentials | Update `config.h` and reflash |
| WiFi keeps disconnecting | Weak signal | Move router closer or add external antenna |
| All relays ON at boot | Boot pin conflict | Avoid GPIO12 (strapping pin) for relays |
| Status LED always on | GPIO2 used as relay | Use GPIO2 only for status LED |
| Buzzer silent | Wrong polarity | Buzzer must be active-high type |
| Random relay toggles | EMI on long wires | Add 100nF capacitor at switch terminals |

### Serial Debug

Monitor the UART output to see real-time events:

```
I (1234) HA_MAIN: Relay1 -> ON
I (2345) HA_MAIN: Sensor: T=25.0C  H=50.0%
I (3456) WIFI: retry
I (4567) HA_MAIN: Switch1 toggle -> Relay1 OFF
```

---

## Schematic

- KiCad file: `home_automation.kicad_sch`
- High-res PNG: `schematic_diagram.png`

Ensure the schematic pin mapping matches `config.h` before PCB fabrication.

---
