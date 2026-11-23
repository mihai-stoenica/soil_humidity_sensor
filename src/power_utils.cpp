#include "power_utils.h"
#include "config.h"
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <sys/time.h>

RTC_DATA_ATTR static uint64_t nextWakeTargetUs = 0;

static uint64_t getRtcTimeUs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

void initPower() {
  analogSetPinAttenuation(Config::PG_PIN, ADC_11db);
}

bool isUsbPowered() {
  pinMode(Config::PG_PIN, INPUT_PULLUP);
  rtc_gpio_pullup_en((gpio_num_t)Config::PG_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)Config::PG_PIN);
  delayMicroseconds(500);

  uint32_t sumMv = 0;
  constexpr int SAMPLES = 4;
  for (int i = 0; i < SAMPLES; i++) {
    sumMv += analogReadMilliVolts(Config::PG_PIN);
    delayMicroseconds(100);
  }

  rtc_gpio_pullup_dis((gpio_num_t)Config::PG_PIN);
  pinMode(Config::PG_PIN, INPUT);

  int avgMv = sumMv / SAMPLES;
  return (avgMv < Config::USB_PG_THRESHOLD_MV);
}

bool isButtonWakeup() {
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
}

bool isScheduledWakeDue() {
  if (nextWakeTargetUs == 0) return true;
  return getRtcTimeUs() >= nextWakeTargetUs;
}

void markScheduledWakeCompleted() {
  uint64_t nowUs = getRtcTimeUs();
  if (nextWakeTargetUs == 0) {
    nextWakeTargetUs = nowUs + Config::BATTERY_SLEEP_INTERVAL_US;
  } else {
    while (nextWakeTargetUs <= nowUs) {
      nextWakeTargetUs += Config::BATTERY_SLEEP_INTERVAL_US;
    }
  }
}

void enterBatteryDeepSleep() {
  uint64_t nowUs = getRtcTimeUs();
  if (nextWakeTargetUs <= nowUs) {
    markScheduledWakeCompleted();
  }
  uint64_t sleepUs = nextWakeTargetUs - nowUs;

  while (digitalRead(Config::BUTTON_PIN) == LOW) {
    delay(10);
  }

  esp_sleep_enable_timer_wakeup(sleepUs);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)Config::BUTTON_PIN, 0);
  esp_deep_sleep_start();
}
