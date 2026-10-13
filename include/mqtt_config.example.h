#pragma once

#include <stdint.h>

constexpr char MQTT_HOST[] = "192.168.178.10";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USER[] = "your-mqtt-user";
constexpr char MQTT_PASSWORD[] = "your-mqtt-password";

constexpr char MQTT_CO2_TOPIC[] = "env-sensor-station/sensor/scd41_co2/state";
constexpr char MQTT_AMBIENT_LIGHT_TOPIC[] =
    "env-sensor-station/sensor/ambient_light/state";
