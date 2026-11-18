#include "humidity_utils.h"
#include "http_utils.h"
#include "mqtt_utils.h"

const int dryValue = 4000;
const int wetValue = 0;

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

  int rawLight = analogRead(Config::LIGHT_PIN);
  data.lightLevel = constrain(map(rawLight, 0, 4095, 0, 100), 0, 100);

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
