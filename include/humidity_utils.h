#ifndef HUMIDITY_UTILS_H
#define HUMIDITY_UTILS_H
      
#define DHTTYPE DHT11   

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

extern DHT dht;

struct SensorData {
  int humidity;
  float temperature;
  int lightLevel;
  bool isValid;
};

SensorData takeSample();
int getSmoothedHumidity();
bool shouldSend(SensorData& data);
void tryUpdateLiveData();
void sendRecord();

#endif