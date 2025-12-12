#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "secrets.h"

using namespace websockets;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const String apiKey = DEVICE_API_KEY;
const String origin = ORIGIN;
const String ws_url = WS_URL;

WebsocketsClient client;

bool stompConnected = false;

void sendStomp(String frame) {
  size_t len = frame.length();
  
  char* buf = new char[len + 1];
  
  memcpy(buf, frame.c_str(), len);
  
  buf[len] = '\0'; 
  
  client.send(buf, len + 1);
  
  delete[] buf;
}

void onMessage(WebsocketsMessage message) {
  String data = message.data();
  Serial.println("WS RAW: " + data);

  if (data.startsWith("CONNECTED")) {
    stompConnected = true;
    Serial.println("[STOMP] Connected!");

    sendStomp(
      "SUBSCRIBE\n"
      "id:sub-0\n"
      "destination:/topic/device\n"
      "\n"
    );
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) delay(100);

  client.onMessage(onMessage);

  client.addHeader("X-API-KEY", apiKey);
  client.addHeader("Origin", origin);
  bool ok = client.connect(ws_url);

    if (!ok) {
    Serial.println("WS FAILED");
    return;
  }

  Serial.println("WS Connected — sending STOMP CONNECT");

  // Send STOMP CONNECT frame
  sendStomp(
    "CONNECT\n"
    "accept-version:1.2\n"
    "host:esp32\n"
    "\n"
  );
}

unsigned long lastSend = 0;         
const unsigned long interval = 10000;

void loop() {
  client.poll();  // keep connection alive

    if(!stompConnected) return;
    
  unsigned long now = millis();

  if (now - lastSend >= interval) {
    lastSend = now;

    sendStomp(
      "SEND\n"
      "destination:/app/device\n"
      "\n"
      "Hello from ESP32!"
    );
    Serial.println("Sent STOMP message");
  }
}
