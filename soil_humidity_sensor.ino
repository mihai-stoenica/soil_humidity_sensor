#include <Arduino.h>
#include "secrets.h"
#include "socket_utils.h"
#include "wifi_utils.h"
#include "command_utils.h"
#include "humidity_utils.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const String apiKey = DEVICE_API_KEY;
const String origin = ORIGIN;
const String ws_url = WS_URL;

unsigned long lastSentSample = 0;
const unsigned long sampleInterval = 2000;

unsigned long lastSentRecord = 0;
const unsigned recordInterval = 3600000;

void setup() {
  Serial.begin(115200);

  initWifi(ssid, password);
  initWebSocket(apiKey, origin, ws_url);

  sendRecord();
  lastSentRecord = millis();
}

int humidity = 32;

void loop() {
  client.poll();
  dht.begin();
  if (!stompConnected) return;

  unsigned long now = millis();
  if (now - lastSentSample >= sampleInterval) {
    lastSentSample = now;
    tryUpdateHumidity();
  }

  if(now - lastSentRecord >= recordInterval) {
    lastSentRecord = now;
    sendRecord();
  }
  commandLoop(); 
}
