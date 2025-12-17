#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <Arduino.h>
#include <ArduinoWebsockets.h>

using namespace websockets;

extern WebsocketsClient client;
extern bool stompConnected;

void initWebSocket(const String& apiKey, const String& origin, const String& ws_url);
void sendMessageStomp(const String& body);

#endif