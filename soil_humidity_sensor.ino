#include <Arduino.h>
#include "secrets.h"
#include "wifi_utils.h"
#include "command_utils.h"
#include "humidity_utils.h"
#include "mqtt_utils.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const String apiKey = DEVICE_API_KEY;
const String origin = ORIGIN;
const String ws_url = WS_URL;

unsigned long lastHeartbeat = 0;

unsigned long lastSentSample = 0;
const unsigned long sampleInterval = 2000;

unsigned long lastSentRecord = 0;
const unsigned recordInterval = 3600000;

void setup() {
  Serial.begin(115200);

  initWifi(ssid, password);
  initMqtt();

  sendRecord();
  lastSentRecord = millis();
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 
  
  dht.begin();

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
