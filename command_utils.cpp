#include "command_utils.h"

struct ActiveCommand {
  int pin;
  unsigned long endTime;
  bool active;
};

ActiveCommand activeCmd = {-1, 0, false};

static int commandToPin(int command) {
  switch (command) {
    case 1: 
      return PIN_PUMP;
    default: 
      return -1;
  }
}

static void executeCommand(int command, int duration) {
  int pin = commandToPin(command);
  if (pin == -1) return;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  activeCmd.pin = pin;
  activeCmd.endTime = millis() + duration * 1000UL;
  activeCmd.active = true;

  Serial.printf("Pin %d ON for %d seconds\n", pin, duration);
}

void handleCommand(const String& json) {
  StaticJsonDocument<128> doc;

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("JSON parse failed");
    return;
  }

  int command = doc["command"];
  int duration = doc["duration"];

  executeCommand(command, duration);
  
}

void commandLoop() {
  if (activeCmd.active && millis() > activeCmd.endTime) {
    digitalWrite(activeCmd.pin, LOW);
    activeCmd.active = false;
    Serial.println("Command finished");
  }
}