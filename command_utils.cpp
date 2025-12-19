#include "command_utils.h"

struct ActiveCommand {
  int pin;
  unsigned long endTime;
  bool active;

  int durationSec = 0;  
  int delaySec = 0;     
  int stepsLeft = 0;    
  bool isWatering = false;
};

ActiveCommand activeCmd = {-1, 0, false, 0, 0, 0, false};

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

  activeCmd.pin = pin;
  activeCmd.durationSec = duration;
  activeCmd.delaySec = 0;    
  activeCmd.stepsLeft = 1;    
  activeCmd.active = true;
  activeCmd.isWatering = true; 

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  activeCmd.endTime = millis() + (duration * 1000UL);
  Serial.printf("START CONTINUOUS: Pin %d ON for %d s\n", pin, duration);
}

static void executeCommand(int command, int duration, int steps, int delay) {
  int pin = commandToPin(command);
  if (pin == -1) return;

  activeCmd.pin = pin;
  activeCmd.durationSec = duration;
  activeCmd.delaySec = delay;
  activeCmd.stepsLeft = steps; 
  activeCmd.active = true;
  activeCmd.isWatering = true; 

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH); 
    
  activeCmd.endTime = millis() + (duration * 1000UL);
  Serial.printf("START STEP: Pin %d ON for %d s. Steps: %d\n", pin, duration, steps);
}

void handleCommand(const String& json) {
  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("JSON parse failed");
    return;
  }

  int command = doc["command"];
  int duration = doc["duration"];
  String pattern = doc["pattern"];

  if(pattern == "continuous") {
    executeCommand(command, duration);
    Serial.println("Pattern cont");
  } else if(pattern == "step") {
    int steps = doc["steps"];
    int delay = doc["delay"];
    executeCommand(command, duration, steps, delay);
    Serial.println("Pattern step");
  }

  
}

void commandLoop() {
  if (!activeCmd.active) return; 

    if (millis() >= activeCmd.endTime) {
        
        if (activeCmd.isWatering) {
            digitalWrite(activeCmd.pin, LOW); 
            activeCmd.stepsLeft--; 

            if (activeCmd.stepsLeft > 0) {
                activeCmd.isWatering = false; 
                activeCmd.endTime = millis() + (activeCmd.delaySec * 1000UL);
                Serial.printf("WAIT: Cooling down for %d s\n", activeCmd.delaySec);
            } else {
                activeCmd.active = false;
                Serial.println("DONE: Finished.");
            }

        } else {
            digitalWrite(activeCmd.pin, HIGH); 
            activeCmd.isWatering = true; 
            activeCmd.endTime = millis() + (activeCmd.durationSec * 1000UL);
            Serial.printf("STEP: Pin %d ON for %d s. Steps left: %d\n", activeCmd.pin, activeCmd.durationSec, activeCmd.stepsLeft);
        }
    }
}