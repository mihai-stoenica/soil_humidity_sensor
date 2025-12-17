#include "socket_utils.h"
#include "secrets.h"
#include "command_utils.h"

WebsocketsClient client;     
bool stompConnected = false;

String convertMessageToJson(String& data) {
  int jsonStart = data.indexOf("\n\n");
  if (jsonStart == -1) return "";

  String json = data.substring(jsonStart + 2);
  json.trim();

  return json; 
}

void sendStomp(String frame) {
  size_t len = frame.length();
  char* buf = new char[len + 1];
  memcpy(buf, frame.c_str(), len);
  buf[len] = '\0';
  client.send(buf, len + 1);
  delete[] buf;
}

void connectStomp() {
  Serial.println("WS Connected — sending STOMP CONNECT");

  sendStomp(
    "CONNECT\n"
    "accept-version:1.2\n"
    "host:esp32\n"
    "\n"
  );
}

void subscribeStomp() {
  sendStomp(
      "SUBSCRIBE\n"
      "id:sub-0\n"
      "destination:/topic/user/" DEVICE_API_KEY "\n"
      "\n"
    );
}

void sendMessageStomp(const String& body) {
  sendStomp(
      "SEND\n"
      "destination:/app/device\n"
      "\n" + 
      body
    );
}

void onMessage(WebsocketsMessage message) {
  String data = message.data();

  if (data.startsWith("CONNECTED")) {
    stompConnected = true;
    Serial.println("[STOMP] Connected!");
  } else if(data.startsWith("MESSAGE")) {
    String payload = convertMessageToJson(data); 
    if(payload != "")
      handleCommand(payload);
    else 
      return;
  }
}

void initWebSocket(const String& apiKey, const String& origin, const String& ws_url) {
  client.onMessage(onMessage);
  
  client.addHeader("X-API-KEY", apiKey);
  client.addHeader("Origin", origin);

  bool ok = client.connect(ws_url);

  if (!ok) {
    Serial.println("WS FAILED");
    return;
  }

  connectStomp();
  subscribeStomp();
  
}