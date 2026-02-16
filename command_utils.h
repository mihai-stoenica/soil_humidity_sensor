#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define PIN_PUMP 33

void handleCommand(const String& json);
void commandLoop();

#endif