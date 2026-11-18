#include "status.h"
#include <Arduino.h>

static int ledPin;
static unsigned long lastBlink = 0;
static const unsigned long wifiDownBlinkInterval = 150;
static const unsigned long mqttDownBlinkInterval = 700;

void initStatusLed(int pin) {
  ledPin = pin;
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void updateStatusLed(ConnState state, unsigned long now) {
  switch (state) {
    case ConnState::WIFI_DOWN:
      if (now - lastBlink >= wifiDownBlinkInterval) {
        lastBlink = now;
        digitalWrite(ledPin, !digitalRead(ledPin));
      }
      break;
    case ConnState::MQTT_DOWN:
      if (now - lastBlink >= mqttDownBlinkInterval) {
        lastBlink = now;
        digitalWrite(ledPin, !digitalRead(ledPin));
      }
      break;
    case ConnState::CONNECTED:
      digitalWrite(ledPin, HIGH);
      break;
  }
}