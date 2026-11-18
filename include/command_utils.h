#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

void handleCommand(const String& json);
void commandLoop();

#endif