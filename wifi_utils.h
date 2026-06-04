#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <WiFi.h>

extern WiFiClient espClient;
extern bool bleActive;

void initWifi();
void handleWifiLoop();

#endif 