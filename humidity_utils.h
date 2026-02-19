#ifndef HUMIDITY_UTILS_H
#define HUMIDITY_UTILS_H

#define SENSOR_PIN 34
#define DHTPIN 5        
#define DHTTYPE DHT11   

#include <Arduino.h>
#include "socket_utils.h"
#include <DHT.h>

extern DHT dht;

int takeSample();
int getSmoothedHumidity();
bool shouldSend(int value);
void tryUpdateHumidity();
void sendRecord();

#endif