#ifndef HUMIDITY_UTILS_H
#define HUMIDITY_UTILS_H
      
#define DHTTYPE DHT11   

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

extern DHT dht;

int takeSample();
int getSmoothedHumidity();
bool shouldSend(int value);
void tryUpdateHumidity();
void sendRecord();

#endif