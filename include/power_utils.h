#pragma once

#include <Arduino.h>

void initPower();
bool isUsbPowered();
bool isButtonWakeup();
bool isScheduledWakeDue();
void markScheduledWakeCompleted();
void enterBatteryDeepSleep();
