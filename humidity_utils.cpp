#include "humidity_utils.h"

const int dryValue = 4000;
const int wetValue = 900;

static float smoothedHumidity = -1;   
static int lastSentHumidity = -1;

const float ALPHA = 0.2;                

int takeSample() {
  int rawValue = analogRead(SENSOR_PIN);

  Serial.print("Raw ADC Value: ");
  Serial.print(rawValue);

  int humidity = map(rawValue, dryValue, wetValue, 0, 100);
  humidity = constrain(humidity, 0, 100);
  
  Serial.printf("Sample : %d%%\n",  humidity);

  return humidity;
}

int getSmoothedHumidity() {
  int raw = takeSample();

  if (smoothedHumidity < 0) {
    smoothedHumidity = raw;
  } else {
    smoothedHumidity = ALPHA * raw + (1.0 - ALPHA) * smoothedHumidity;
  }

  return round(smoothedHumidity);
}

bool shouldSend(int value) {
  if (lastSentHumidity < 0) {
    lastSentHumidity = value;
    return true;
  }

  if (value != lastSentHumidity) {
    lastSentHumidity = value;
    return true;
  }

  return false;
}

void sendHumidity(int humidity) {
  if(humidity > 100 || humidity < 0) return;

  String payload = "{ \"humidity\": " + String(humidity) + " }";
  sendMessageStomp(payload);
}

void tryUpdateHumidity() {
  int humidity = getSmoothedHumidity();

  if(shouldSend(humidity)) {
    sendHumidity(humidity);
    Serial.printf("📡 Sent humidity: %d%%\n", humidity);
  }
}