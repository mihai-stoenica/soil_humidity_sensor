#include "command_utils.h"

enum class CmdPhase {
  IDLE,
  WATERING,
  COOLDOWN
};

struct ActiveCommand {
  int pin = -1;
  unsigned long endTime = 0;
  CmdPhase phase = CmdPhase::IDLE;

  int durationSec = 0;  
  int delaySec = 0;     
  int stepsLeft = 0;    
};

ActiveCommand activeCmd;

static int commandToPin(int command) {
  switch (command) {
    case 1: 
      return Config::PUMP_PIN;
    default: 
      return -1;
  }
}

static bool executeCommand(int command, int duration, int steps = 1, int delay = 0) {
  if(activeCmd.phase != CmdPhase::IDLE) {
    Serial.println("A command is already active. Ignoring new command.");
    return false;
  }


  if(duration <= 0 || steps <= 0) {
    Serial.println("Invalid duration or steps. Ignoring command.");
    return false;
  }

  int pin = commandToPin(command);

  if (pin == -1) {
    Serial.println("Invalid command. Ignoring.");
    return false;
  }

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  activeCmd.pin = pin;
  activeCmd.durationSec = duration;
  activeCmd.delaySec = delay;
  activeCmd.stepsLeft = steps;
  activeCmd.phase = CmdPhase::WATERING;
  activeCmd.endTime = millis() + (duration * 1000UL);

  Serial.printf("START: Pin %d ON for %d s. Steps: %d, Delay: %d s\n",
                pin, duration, steps, delay);

  return true;
}

void handleCommand(const String& json) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("JSON parse failed");
    return;
  }

  int command = doc["command"] | -1;
  int duration = doc["duration"] | 0;
  String pattern = doc["pattern"] | "";

  if(pattern == "continuous") {
    executeCommand(command, duration);
    Serial.println("Pattern cont");
  } else if(pattern == "step") {
    int steps = doc["steps"] | 1;
    int delay = doc["delay"] | 0;
    executeCommand(command, duration, steps, delay);
    Serial.println("Pattern step");
  } else {
    Serial.println("Unknown pattern. Ignoring command.");
  }

}

void commandLoop() {
  if (activeCmd.phase == CmdPhase::IDLE) return; 

  if (millis() < activeCmd.endTime) return;

  switch(activeCmd.phase) {
    case CmdPhase::WATERING:
      digitalWrite(activeCmd.pin, LOW);
      Serial.printf("STOP: Pin %d OFF\n", activeCmd.pin);

      activeCmd.stepsLeft--;

      if (activeCmd.stepsLeft > 0) {
        activeCmd.phase = CmdPhase::COOLDOWN;
        activeCmd.endTime = millis() + (activeCmd.delaySec * 1000UL);
        Serial.printf("Cooldown for %d s. Steps left: %d\n", activeCmd.delaySec, activeCmd.stepsLeft);
      } else {
        activeCmd.phase = CmdPhase::IDLE;
        Serial.println("Command completed.");
      }
      break;

    case CmdPhase::COOLDOWN:
      digitalWrite(activeCmd.pin, HIGH);
      activeCmd.phase = CmdPhase::WATERING;
      activeCmd.endTime = millis() + (activeCmd.durationSec * 1000UL);
      Serial.printf("START: Pin %d ON for %d s. Steps left: %d\n",
                    activeCmd.pin, activeCmd.durationSec, activeCmd.stepsLeft);
      break;

    default:
      break;
  }
}