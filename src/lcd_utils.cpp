#include "lcd_utils.h"
#include "config.h"
#include "humidity_utils.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

static Adafruit_ST7735 tft(Config::TFT_CS_PIN, Config::TFT_RS_PIN, Config::TFT_RST_PIN);

static bool lcdActive = false;
static unsigned long lcdTurnedOnAt = 0;
static unsigned long lastLcdRefresh = 0;

static bool lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;
static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;

static void backlightWrite(bool on) {
  digitalWrite(
    Config::TFT_BLK_PIN,
    on ? Config::TFT_BL_ON_LEVEL : (Config::TFT_BL_ON_LEVEL == HIGH ? LOW : HIGH)
  );
}

static void setDisplaySleep(bool sleep) {
  tft.enableDisplay(!sleep);
  tft.sendCommand(sleep ? ST77XX_SLPIN : ST77XX_SLPOUT);
  backlightWrite(!sleep);
  delay(5);
}

static void renderSensorData() {
  SensorData data = takeSample();

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 8);
  tft.print("PLANT MONITOR");
  tft.drawFastHLine(0, 20, tft.width(), ST77XX_CYAN);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 30);
  tft.print("Soil Moisture:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(8, 42);
  tft.printf("%d %%", data.humidity);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 68);
  tft.print("Temperature:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(8, 80);
  if (data.isValid) {
    tft.printf("%.1f C", data.temperature);
  } else {
    tft.print("--.- C");
  }

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 106);
  tft.print("Light Level:");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_ORANGE);
  tft.setCursor(8, 118);
  tft.printf("%d %%", data.lightLevel);
}

void initLcd() {
  pinMode(Config::BUTTON_PIN, INPUT); 
  pinMode(Config::TFT_BLK_PIN, OUTPUT);
  backlightWrite(false);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  setDisplaySleep(true);
  lcdActive = false;
}

void handleLcdLoop(unsigned long now) {
  bool reading = digitalRead(Config::BUTTON_PIN);

  if (reading != lastButtonState) {
    if (now - lastDebounceTime > DEBOUNCE_DELAY_MS) {
      lastDebounceTime = now;
      lastButtonState = reading;

      if (reading == LOW) {
        lcdTurnedOnAt = now;
        lastLcdRefresh = now;

        if (!lcdActive) {
          lcdActive = true;
          setDisplaySleep(false);
        }
        renderSensorData();
      }
    }
  }

  if (lcdActive) {
    if (now - lcdTurnedOnAt >= Config::LCD_ON_DURATION_MS) {
      tft.fillScreen(ST77XX_BLACK);
      setDisplaySleep(true);
      lcdActive = false;
    } else if (now - lastLcdRefresh >= Config::LCD_REFRESH_INTERVAL_MS) {
      lastLcdRefresh = now;
      renderSensorData();
    }
  }
}
