#include "wifi_utils.h"

void initWifi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(100);

  Serial.println("Wifi connected!");
}