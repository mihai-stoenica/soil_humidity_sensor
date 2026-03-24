#include "mqtt_utils.h"
#include "secrets.h"
#include "command_utils.h"

const char* mqtt_server = BASE_URL;

const char* user = DEVICE_API_KEY;
const char* pwd = DEVICE_SECRET;

PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println(message);

  handleCommand(message);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("Device-" DEVICE_API_KEY, user, pwd, "soil/device/" DEVICE_API_KEY "/status", 1, true, "offline")) {
      client.publish("soil/device/" DEVICE_API_KEY "/status", "online", true);
      Serial.println("connected");
      client.subscribe("soil/device/" DEVICE_API_KEY "/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void sendMessage(String& payload) {
  client.publish("soil/device/" DEVICE_API_KEY "/telemetry", payload.c_str(), true);
}

void initMqtt() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}