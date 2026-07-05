#include "status.h"
#include <Arduino.h>

static int ledPin;
static unsigned long lastBlink = 0;
static const unsigned long blinkInterval = 500;

void initStatusLed(int pin) {
  ledPin = pin;
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void updateStatusLed(ConnState state, unsigned long now) {
  switch (state) {
    case ConnState::WIFI_DOWN:
      digitalWrite(ledPin, LOW);
      break;
    case ConnState::MQTT_DOWN:
      if (now - lastBlink >= blinkInterval) {
        lastBlink = now;
        digitalWrite(ledPin, !digitalRead(ledPin));
      }
      break;
    case ConnState::CONNECTED:
      digitalWrite(ledPin, HIGH);
      break;
  }
}