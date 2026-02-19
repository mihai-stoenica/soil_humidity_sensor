#include "http_utils.h"
#include "secrets.h"

void POST(const char* path, const String& jsonBody) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(API_URL) + path;
  http.begin(url);


  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-KEY", DEVICE_API_KEY);

  int httpResponseCode = http.POST(jsonBody);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("POST Response:");
    Serial.println(response);
  } else {
    Serial.print("POST Error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
