#ifndef HUMIDITY_UTILS_H
#define HUMIDITY_UTILS_H

#define SENSOR_PIN 34

#include <Arduino.h>
#include "socket_utils.h"

int takeSample();
int getSmoothedHumidity();
bool shouldSend(int value);
void tryUpdateHumidity();

#endif