#include "lcd_utils.h"
#include "config.h"
#include "humidity_utils.h"
#include "wifi_utils.h"
#include "mqtt_utils.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

static Adafruit_ST7735 tft(Config::TFT_CS_PIN, Config::TFT_RS_PIN, Config::TFT_RST_PIN);

static bool lcdActive = false;
static unsigned long lcdTurnedOnAt = 0;
static unsigned long lastLcdRefresh = 0;

static bool lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;
static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;

static bool buttonHeld = false;
static unsigned long buttonPressStart = 0;
static bool resetTriggeredDuringHold = false;
static int lastRenderedHoldStage = -1;
static unsigned long lastHoldBarDraw = 0;

static void backlightWrite(bool on) {
  int level = on ? Config::TFT_BL_ON_LEVEL : (Config::TFT_BL_ON_LEVEL == HIGH ? LOW : HIGH);
  gpio_num_t pin = (gpio_num_t)Config::TFT_BLK_PIN;
  rtc_gpio_hold_dis(pin);
  gpio_hold_dis(pin);
  if (rtc_gpio_is_valid_gpio(pin)) {
    rtc_gpio_deinit(pin);
  }
  pinMode(Config::TFT_BLK_PIN, OUTPUT);
  digitalWrite(Config::TFT_BLK_PIN, level);
}

static void setDisplaySleep(bool sleep) {
  tft.enableDisplay(!sleep);
  tft.sendCommand(sleep ? ST77XX_SLPIN : ST77XX_SLPOUT);
  backlightWrite(!sleep);
  delay(5);
}

static void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
  int clamped = constrain(percent, 0, 100);
  int fillW = ((w - 2) * clamped) / 100;
  tft.drawRect(x, y, w, h, 0x4208);
  if (fillW > 0) {
    tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
  }
}

static void renderHoldBanner(const char* text, uint16_t color, int progressPct) {
  tft.fillRect(0, 120, tft.width(), 40, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(6, 128);
  tft.print(text);
  drawProgressBar(6, 142, tft.width() - 12, 10, progressPct, color);
}

static void renderSensorData(bool useCached = false) {
  SensorData data = (useCached && hasCachedSensorData()) ? getCachedSensorData() : takeSample();
  bool wifiUp = (WiFi.status() == WL_CONNECTED);
  bool serverUp = (wifiUp && client.connected());

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(22, 6);
  tft.print("PLANT MONITOR");
  tft.drawFastHLine(0, 17, tft.width(), ST77XX_CYAN);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(6, 22);
  tft.print("Soil Moisture:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(6, 33);
  tft.printf("%d%%", data.humidity);
  drawProgressBar(72, 37, 50, 8, data.humidity, ST77XX_GREEN);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(6, 54);
  tft.print("Temperature:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(6, 65);
  if (data.isValid) {
    tft.printf("%.1f C", data.temperature);
  } else {
    tft.print("--.- C");
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(6, 86);
  tft.print("Light Level:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_ORANGE);
  tft.setCursor(6, 97);
  tft.printf("%d%%", data.lightLevel);
  drawProgressBar(72, 101, 50, 8, data.lightLevel, ST77XX_ORANGE);

  tft.drawFastHLine(0, 118, tft.width(), 0x4208);

  uint16_t wifiColor = bleActive ? ST77XX_CYAN : (wifiUp ? ST77XX_GREEN : ST77XX_RED);
  tft.fillCircle(10, 128, 3, wifiColor);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(18, 125);
  tft.print("WiFi: ");
  tft.setTextColor(wifiColor);
  tft.print(bleActive ? "BLE Setup" : (wifiUp ? "Connected" : "Offline"));

  uint16_t srvColor = serverUp ? ST77XX_GREEN : ST77XX_RED;
  tft.fillCircle(10, 142, 3, srvColor);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(18, 139);
  tft.print("Srv:  ");
  tft.setTextColor(srvColor);
  tft.print(serverUp ? "Online" : "Unconnected");

  unsigned long elapsed = millis() - lcdTurnedOnAt;
  int remainPct = 100 - constrain((int)((elapsed * 100UL) / Config::LCD_ON_DURATION_MS), 0, 100);
  int barW = (tft.width() * remainPct) / 100;
  tft.fillRect(0, 158, barW, 2, ST77XX_CYAN);
}

void initLcd() {
  pinMode(Config::BUTTON_PIN, INPUT_PULLUP); 
  backlightWrite(false);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  setDisplaySleep(true);
  lcdActive = false;
}

void wakeLcd(unsigned long now, bool useCached) {
  lcdTurnedOnAt = now;
  lastLcdRefresh = now;
  lastDebounceTime = now;
  lastButtonState = digitalRead(Config::BUTTON_PIN);

  if (lastButtonState == LOW) {
    buttonHeld = true;
    buttonPressStart = now;
    resetTriggeredDuringHold = false;
    lastRenderedHoldStage = -1;
  }

  if (!lcdActive) {
    lcdActive = true;
    setDisplaySleep(false);
  }
  renderSensorData(useCached);
}

bool isLcdActive() {
  return lcdActive;
}

void handleLcdLoop(unsigned long now, bool useCached) {
  bool reading = digitalRead(Config::BUTTON_PIN);

  if (reading != lastButtonState) {
    if (now - lastDebounceTime > DEBOUNCE_DELAY_MS) {
      lastDebounceTime = now;
      lastButtonState = reading;

      if (reading == LOW) {
        buttonHeld = true;
        buttonPressStart = now;
        resetTriggeredDuringHold = false;
        lastRenderedHoldStage = -1;
        lcdTurnedOnAt = now;
        lastLcdRefresh = now;

        if (!lcdActive) {
          lcdActive = true;
          setDisplaySleep(false);
        }
        renderSensorData(useCached);
      } else {
        if (buttonHeld) {
          buttonHeld = false;
          lcdTurnedOnAt = now;
          lastLcdRefresh = now;
          renderSensorData(useCached);
        }
      }
    }
  }

  if (buttonHeld) {
    lcdTurnedOnAt = now;
    unsigned long heldMs = now - buttonPressStart;

    if (!resetTriggeredDuringHold && heldMs >= Config::BTN_HOLD_PROVISION_MS) {
      resetTriggeredDuringHold = true;
      resetWifiAndStartBle();
      renderHoldBanner("BLE PROVISIONING!", ST77XX_CYAN, 100);
      lastRenderedHoldStage = 2;
    } else if (!resetTriggeredDuringHold && heldMs >= 700) {
      int pct = constrain((int)(((heldMs - 700UL) * 100UL) / (Config::BTN_HOLD_PROVISION_MS - 700UL)), 0, 100);
      if (lastRenderedHoldStage != 1 || now - lastHoldBarDraw >= 150) {
        lastHoldBarDraw = now;
        lastRenderedHoldStage = 1;
        renderHoldBanner("Hold: BLE Setup...", ST77XX_CYAN, pct);
      }
    }
    return;
  }

  if (lcdActive) {
    if (now - lcdTurnedOnAt >= Config::LCD_ON_DURATION_MS) {
      tft.fillScreen(ST77XX_BLACK);
      setDisplaySleep(true);
      lcdActive = false;
    } else if (now - lastLcdRefresh >= Config::LCD_REFRESH_INTERVAL_MS) {
      lastLcdRefresh = now;
      renderSensorData(useCached);
    }
  }
}
