#include <Arduino.h>
#include "secrets.h"
#include "wifi_utils.h"
#include "command_utils.h"
#include "humidity_utils.h"
#include "mqtt_utils.h"
#include "status.h"
#include "config.h"


unsigned long lastSentSample = 0;
unsigned long lastSentRecord = 0;
unsigned long lastMqttReconnectAttempt = 0;

void setup() {
  Serial.begin(115200);

  pinMode(Config::PUMP_PIN, OUTPUT);
  digitalWrite(Config::PUMP_PIN, HIGH);

  pinMode(Config::LED_PIN, OUTPUT);
  digitalWrite(Config::LED_PIN, LOW);

  initWifi();
  initMqtt();
  dht.begin();

  if(WiFi.status() == WL_CONNECTED) {
    sendRecord();
    lastSentRecord = millis();
  }

}

void loop() {
  unsigned long now = millis();

  handleWifiLoop();

  ConnState state = WiFi.status() != WL_CONNECTED 
                      ? ConnState::WIFI_DOWN 
                      : (!client.connected() 
                            ? ConnState::MQTT_DOWN 
                            : ConnState::CONNECTED);

  updateStatusLed(state, now);

  switch(state) {
    case ConnState::WIFI_DOWN:
    break;

    case ConnState::MQTT_DOWN:
      if (now - lastMqttReconnectAttempt > Config::MQTT_RETRY_INTERVAL_MS) {
        lastMqttReconnectAttempt = now;
        reconnect();
      }
      break;

    case ConnState::CONNECTED:
      if (now - lastSentSample >= Config::SAMPLE_INTERVAL_MS) {
        lastSentSample = now;
        tryUpdateHumidity();
      }
      if (now - lastSentRecord >= Config::RECORD_INTERVAL_MS) {
        lastSentRecord = now;
        sendRecord();
      }
      commandLoop();
      break;
  }
}
