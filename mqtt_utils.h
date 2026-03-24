#ifndef MQTT_UTILS_H
#define MQTT_UTILS_H

#include <Wifi.h>
#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient client;

void initMqtt();
void reconnect();
void sendMessage(String& payload);

#endif