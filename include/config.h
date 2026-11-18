#pragma once

namespace Config {
  constexpr int LED_PIN = 13;
  constexpr int PUMP_PIN = 33;
  constexpr int HUMIDITY_PIN = 34;
  constexpr int DHTPIN = 5;  
  constexpr int LIGHT_PIN = 32;

  constexpr unsigned long DB_SAVE_INTERVAL_MS = 3600000;
  constexpr unsigned long LIVE_CHECK_INTERVAL_MS = 20000;
  constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 5000;
}