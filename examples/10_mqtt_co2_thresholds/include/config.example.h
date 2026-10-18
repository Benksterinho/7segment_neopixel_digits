#pragma once

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_HOST "YOUR_MQTT_BROKER"
#define MQTT_PORT 1883
#define MQTT_USER "YOUR_MQTT_USER"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#define MQTT_CO2_TOPIC "env-sensor-station/sensor/scd41_co2/state"

// Illustrative display color boundaries in ppm; these are not safety limits.
#define CO2_AMBER_THRESHOLD_PPM 800
#define CO2_RED_THRESHOLD_PPM 1200
