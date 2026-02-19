#include "humidity_utils.h"
#include "http_utils.h"

const int dryValue = 4000;
const int wetValue = 1000;

static float smoothedHumidity = -1;   

static int lastSentHumidity = -1;
static float lastSentTemp = -100.0;

bool firstRun = true;

const float ALPHA = 0.2;                

DHT dht(DHTPIN, DHTTYPE);

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

bool shouldSend(int humidity, float temp) {
  if (firstRun) {
    lastSentHumidity = humidity;
    lastSentTemp = temp;
    firstRun = false;
    return true;
  }


  bool humidityChanged = (humidity != lastSentHumidity);
  bool tempChanged = (fabs(lastSentTemp - temp) >= 0.5);

  if (humidityChanged || tempChanged) {
    lastSentHumidity = humidity;
    lastSentTemp = temp;
    return true;
  }

  return false;
}

void sendHumidityAndTemp(int humidity, float temp) {
  if(humidity > 100 || humidity < 0) return;

  String payload = 
    "{ \"humidity\": " + String(humidity) + 
    ", \"temperature\": " + String(temp) + 
    " }";
  sendMessageStomp(payload);
}

void tryUpdateHumidity() {
  int humidity = getSmoothedHumidity();
  float temperature = dht.readTemperature();

  if (isnan(temperature)) return;

  if(shouldSend(humidity, temperature)) {
    sendHumidityAndTemp(humidity, temperature);
    Serial.printf("📡 Sent soil: %d%% | Temp: %.1f°C\n",
                  humidity, temperature);
  }
}

void sendRecord() {
  int humidity = takeSample();

  String json = "{\"value\":" + String(humidity) + "}";

  POST("/humidity/save", json);
}
