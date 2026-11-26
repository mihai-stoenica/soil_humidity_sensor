#include <Arduino.h>
#include "secrets.h"
#include "wifi_utils.h"
#include "command_utils.h"
#include "humidity_utils.h"
#include "mqtt_utils.h"
#include "status.h"
#include "config.h"
#include "lcd_utils.h"
#include "power_utils.h"

unsigned long lastSentSample = 0;
unsigned long lastSentRecord = 0;
unsigned long lastMqttReconnectAttempt = 0;
int mqttRetryCount = 0;

static void runBatteryScheduledCycle() {
  initStatusLed(Config::LED_PIN);
  initWifi();
  initMqtt();

  for (int i = 0; i < 5; i++) {
    SensorData s = takeSample();
    if (s.isValid) break;
    delay(500);
  }

  while (bleActive && WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    handleWifiLoop();
    updateStatusLed(ConnState::WIFI_DOWN, now);
    handleLcdLoop(now, true);
    if (isUsbPowered()) return;
    delay(10);
  }

  if (WiFi.status() == WL_CONNECTED) {
    sendRecord();
    if (reconnect()) {
      tryUpdateLiveData();
      unsigned long listenStart = millis();
      while (millis() - listenStart < Config::BATTERY_MQTT_WAIT_MS || isCommandActive() || isLcdActive()) {
        unsigned long now = millis();
        client.loop();
        commandLoop();
        updateStatusLed(ConnState::CONNECTED, now);
        handleLcdLoop(now, true);
        if (isUsbPowered()) return;
        delay(10);
      }
    }
  }

  while (isLcdActive()) {
    unsigned long now = millis();
    handleLcdLoop(now, true);
    if (isUsbPowered()) return;
    delay(10);
  }

  digitalWrite(Config::LED_PIN, LOW);
  markScheduledWakeCompleted();
  enterBatteryDeepSleep();
}

static void runBatteryButtonCycle() {
  wakeLcd(millis(), true);

  while (isLcdActive() || bleActive) {
    unsigned long now = millis();
    handleLcdLoop(now, true);
    if (bleActive) {
      static bool ledInited = false;
      if (!ledInited) {
        initStatusLed(Config::LED_PIN);
        ledInited = true;
      }
      handleWifiLoop();
      updateStatusLed(ConnState::WIFI_DOWN, now);
    }
    if (isUsbPowered()) return;
    if (!bleActive && isScheduledWakeDue()) {
      runBatteryScheduledCycle();
      return;
    }
    delay(10);
  }

  if (isScheduledWakeDue()) {
    runBatteryScheduledCycle();
    return;
  }

  enterBatteryDeepSleep();
}

void setup() {
  Serial.begin(115200);

  pinMode(Config::PUMP_PIN, OUTPUT);
  digitalWrite(Config::PUMP_PIN, HIGH);

  initPower();
  initSensors();
  initLcd();

  if (!isUsbPowered()) {
    if (isButtonWakeup() && !isScheduledWakeDue() && hasCachedSensorData()) {
      runBatteryButtonCycle();
    } else {
      runBatteryScheduledCycle();
    }
  }

  initStatusLed(Config::LED_PIN);
  initWifi();
  initMqtt();

  if (WiFi.status() == WL_CONNECTED) {
    sendRecord();
    lastSentSample = millis();
    lastSentRecord = millis();
  }
}

void loop() {
  unsigned long now = millis();

  if (!isUsbPowered() && !bleActive && !isCommandActive() && !isLcdActive()) {
    digitalWrite(Config::LED_PIN, LOW);
    markScheduledWakeCompleted();
    enterBatteryDeepSleep();
  }

  handleWifiLoop();
  client.loop();

  ConnState state = WiFi.status() != WL_CONNECTED 
                      ? ConnState::WIFI_DOWN 
                      : (!client.connected() 
                            ? ConnState::MQTT_DOWN 
                            : ConnState::CONNECTED);

  updateStatusLed(state, now);
  handleLcdLoop(now);
  commandLoop();

  switch(state) {
    case ConnState::WIFI_DOWN:
      mqttRetryCount = 0; 
      break;

    case ConnState::MQTT_DOWN:
      if (mqttRetryCount < Config::MQTT_MAX_RETRIES &&
          now - lastMqttReconnectAttempt > Config::MQTT_RETRY_INTERVAL_MS) {
        lastMqttReconnectAttempt = now;
        mqttRetryCount++;
        Serial.printf("MQTT attempt %d/%d...\n", mqttRetryCount, Config::MQTT_MAX_RETRIES);
        if (reconnect()) {
          mqttRetryCount = 0;
        } else if (mqttRetryCount >= Config::MQTT_MAX_RETRIES) {
          Serial.println("\nMax MQTT retries reached. Stopping MQTT connection attempts.");
        }
      }
      break;

    case ConnState::CONNECTED:
      mqttRetryCount = 0;
      if (now - lastSentSample >= Config::DB_SAVE_INTERVAL_MS) {
        lastSentSample = now;
        sendRecord();
      }
      if (now - lastSentRecord >= Config::LIVE_CHECK_INTERVAL_MS) {
        lastSentRecord = now;
        tryUpdateLiveData();
      }
      break;
  }
}
