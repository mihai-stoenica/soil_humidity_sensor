#pragma once

#include <Arduino.h>

void initLcd();
void handleLcdLoop(unsigned long now, bool useCached = false);
void wakeLcd(unsigned long now, bool useCached = false);
bool isLcdActive();
