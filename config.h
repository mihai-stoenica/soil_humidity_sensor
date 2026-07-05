#pragma once

namespace Config {
  constexpr int LED_PIN = 2;
  constexpr int PUMP_PIN = 33;
  constexpr int HUMIDITY_PIN = 34;
  constexpr int DHTPIN = 5;  

  constexpr unsigned long SAMPLE_INTERVAL_MS = 2000;
  constexpr unsigned long RECORD_INTERVAL_MS = 3600000;
  constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 5000;
}