#include <Arduino.h>
#include "secrets.h"
#include "wifi_utils.h"
#include "command_utils.h"
#include "humidity_utils.h"
#include "mqtt_utils.h"

const String apiKey = DEVICE_API_KEY;
const String origin = ORIGIN;
const String ws_url = WS_URL;

unsigned long lastHeartbeat = 0;

unsigned long lastSentSample = 0;
const unsigned long sampleInterval = 2000;

unsigned long lastSentRecord = 0;
const unsigned recordInterval = 3600000;

const int LED_PIN = 2;

unsigned long lastBlink = 0;
unsigned long lastMqttReconnectAttempt = 0;

void setup() {
  Serial.begin(115200);

  pinMode(33, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(33, HIGH);

  digitalWrite(LED_PIN, LOW);

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

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW);
  } else if (!client.connected()) {
    
    if (now - lastBlink >= 500) {
      lastBlink = now;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
    }

    if (now - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = now;
      if (reconnect()) {
        digitalWrite(LED_PIN, HIGH); 
      }
    }
  } else {
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
}
