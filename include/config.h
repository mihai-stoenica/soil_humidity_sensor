#pragma once

namespace Config {
  constexpr int LED_PIN = 13;
  constexpr int PUMP_PIN = 21;
  constexpr int HUMIDITY_PIN = 34;
  constexpr int DHTPIN = 26;  
  constexpr int LIGHT_PIN = 36; 

  constexpr int BUTTON_PIN = 25;
  constexpr int TFT_BLK_PIN = 32;
  constexpr int TFT_BL_ON_LEVEL = HIGH;
  constexpr int TFT_CLK_PIN = 18;
  constexpr int TFT_SDA_PIN = 23;
  constexpr int TFT_CS_PIN = 4;
  constexpr int TFT_RS_PIN = 16;
  constexpr int TFT_RST_PIN = 17;

  constexpr unsigned long LCD_ON_DURATION_MS = 8000;
  constexpr unsigned long LCD_REFRESH_INTERVAL_MS = 2000;

  constexpr unsigned long DB_SAVE_INTERVAL_MS = 3600000;
  constexpr unsigned long LIVE_CHECK_INTERVAL_MS = 20000;
  constexpr unsigned long MQTT_RETRY_INTERVAL_MS = 5000;
  constexpr int MQTT_MAX_RETRIES = 1;
}