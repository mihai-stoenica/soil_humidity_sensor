#pragma once

enum class ConnState { WIFI_DOWN, MQTT_DOWN, CONNECTED };

void initStatusLed(int pin);
void updateStatusLed(ConnState state, unsigned long now);