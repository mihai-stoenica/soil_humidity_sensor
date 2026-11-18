#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <HTTPClient.h>
#include <WiFi.h>

void POST(const char* url, const String& jsonBody);

#endif