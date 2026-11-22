#include "humidity_utils.h"
#include "http_utils.h"
#include "mqtt_utils.h"

const int dryValue = 4000;
const int wetValue = 0;

const float LDR_PULLDOWN_OHMS = 5100.0f;
const float LDR_VCC_MV        = 3300.0f;
const float LDR_ADC_MAX_MV    = 3145.0f;
const float LDR_DARK_OHMS     = 45000.0f;
const float LDR_BRIGHT_OHMS   = 220.0f;

static int readMilliVoltsAvg(int pin, int n = 8) {
  uint32_t sum = 0;
  for (int i = 0; i < n; i++) {
    sum += analogReadMilliVolts(pin);
    delayMicroseconds(300);
  }
  return sum / n;
}

static int mvToLightPercent(int mv) {
  if (mv <= 50) return 0;
  if (mv >= (int)LDR_ADC_MAX_MV) return 96;

  float rLdr = LDR_PULLDOWN_OHMS * (LDR_VCC_MV - (float)mv) / (float)mv;

  if (rLdr >= LDR_DARK_OHMS) return 0;
  if (rLdr <= LDR_BRIGHT_OHMS) return 100;

  float logDark   = log10f(LDR_DARK_OHMS);
  float logBright = log10f(LDR_BRIGHT_OHMS);
  float logCurr   = log10f(rLdr);

  float pct = (logDark - logCurr) * 100.0f / (logDark - logBright);
  return constrain((int)(pct + 0.5f), 0, 100);
}

void initSensors() {
  dht.begin();
  analogReadResolution(12);
  analogSetPinAttenuation(Config::LIGHT_PIN, ADC_11db);
}

static float smoothedHumidity = -1;   

static int lastSentHumidity = -1;
static float lastSentTemp = -100.0;
static int lastSentLight = -1;

bool firstRun = true;

const float ALPHA = 0.2;                

DHT dht(Config::DHTPIN, DHTTYPE);



SensorData takeSample() {
  SensorData data;

  int rawHum = analogRead(Config::HUMIDITY_PIN);
  int humidity = constrain(map(rawHum, dryValue, wetValue, 0, 100), 0, 100);

  if (smoothedHumidity < 0) {
    smoothedHumidity = humidity;
  } else {
    smoothedHumidity = ALPHA * humidity + (1.0 - ALPHA) * smoothedHumidity;
  }

  data.humidity = round(smoothedHumidity);

  data.temperature = dht.readTemperature();
  data.isValid = !isnan(data.temperature); 

  int lightMv = readMilliVoltsAvg(Config::LIGHT_PIN);
  data.lightLevel = mvToLightPercent(lightMv);

  return data;
}

bool shouldSend(SensorData& data) {
  if (firstRun) {
    lastSentHumidity = data.humidity;
    lastSentTemp = data.temperature;
    lastSentLight = data.lightLevel;
    firstRun = false;
    return true;
  }


  bool humidityChanged = (data.humidity != lastSentHumidity);
  bool tempChanged = (fabs(lastSentTemp - data.temperature) >= 0.5);
  bool lightChanged = (abs(lastSentLight - data.lightLevel) >= 5);

  if (humidityChanged || tempChanged || lightChanged) {
    lastSentHumidity = data.humidity;
    lastSentTemp = data.temperature;
    lastSentLight = data.lightLevel;
    return true;
  }

  return false;
}

void sendLiveData(SensorData& data) {
  if(!data.isValid) return;

  String payload = 
    "{ \"humidity\": " + String(data.humidity) + 
    ", \"temperature\": " + String(data.temperature) + 
    ", \"light_level\": " + String(data.lightLevel) +
    " }";

  sendMessage(payload);
}

void tryUpdateLiveData() {
  SensorData data = takeSample();

  if (!data.isValid) return;

  if(shouldSend(data)) {
    sendLiveData(data);
    Serial.printf("📡 Sent soil: %d%% | Temp: %.1f°C | Light: %d%%\n",
                  data.humidity, data.temperature, data.lightLevel);
  }
}

void sendRecord() {
  SensorData data = takeSample();

  if(!data.isValid) return;

  String json = "{\"humidity\":" + String(data.humidity) + 
                ",\"temperature\":" + String(data.temperature) + 
                ",\"light_level\":" + String(data.lightLevel) + "}";

  POST("/humidity/save", json);
}
