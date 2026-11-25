#include "wifi_utils.h"
#include "config.h"
#include "secrets.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CRED_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STATUS_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define INFO_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26ab"

WiFiClient espClient;
bool bleActive = false;

BLECharacteristic *pStatusChar = nullptr;
BLECharacteristic *pInfoChar = nullptr;
volatile bool keysReadByClient = false;
String jsonBuffer = ""; 
bool newCredentialsReceived = false;
String newSSID = "";
String newPass = "";
unsigned long connectionStartTime = 0;
unsigned long bleStartTime = 0;
bool attemptingConnection = false;

class InfoCharCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) override {
        keysReadByClient = true;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            for (int i = 0; i < rxValue.length(); i++) {
                if (rxValue[i] != '\0') { 
                    jsonBuffer += rxValue[i];
                }
            }

            if (jsonBuffer.indexOf('}') != -1) { 
                Serial.println("Full JSON Received: " + jsonBuffer);

                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, jsonBuffer);

                if (!error) {
                    const char* ssid = doc["ssid"];
                    const char* pass = doc["password"];

                    if (ssid) {
                        Serial.printf("Received credentials for: %s\n", ssid);
                        newSSID = String(ssid);
                        newPass = String(pass ? pass : "");
                        newCredentialsReceived = true; 
                    }
                } else {
                    Serial.print("Parse Error: ");
                    Serial.println(error.c_str());
                }
                jsonBuffer = ""; 
            }
        }
    }
};

void stopBLE() {
    if (!bleActive) return;
    BLEDevice::getAdvertising()->stop();
    BLEDevice::deinit(true);
    bleActive = false;
    Serial.println("BLE stopped and deinitialized.");
}

void startBLE() {
    BLEDevice::init("PROV_MyESP32");
    BLEServer *pServer = BLEDevice::createServer();
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    BLECharacteristic *pCredChar = pService->createCharacteristic(CRED_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pCredChar->setCallbacks(new MyCallbacks());

    pStatusChar = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    pStatusChar->addDescriptor(new BLE2902());

    pInfoChar = pService->createCharacteristic(INFO_CHAR_UUID, BLECharacteristic::PROPERTY_READ);
    String infoPayload = String("{\"apiKey\":\"") + DEVICE_API_KEY + "\",\"secret\":\"" + DEVICE_SECRET + "\"}";
    pInfoChar->setValue(infoPayload.c_str());
    pInfoChar->setCallbacks(new InfoCharCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("BLE Provisioning Active...");
    bleActive = true;
    bleStartTime = millis();
}

void resetWifiAndStartBle() {
    Serial.println("Resetting saved WiFi credentials and starting BLE provisioning...");
    attemptingConnection = false;
    newCredentialsReceived = false;
    jsonBuffer = "";
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(200);
    if (!bleActive) {
        startBLE();
    } else {
        bleStartTime = millis();
    }
}

void initWifi() {
    Serial.println("Checking for saved WiFi...");
    WiFi.begin(); 

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nSuccessfully connected to saved WiFi!");
    } 
    else {
        Serial.println("\nNo valid stored WiFi found. Starting BLE...");
        startBLE();
    }

}

void handleWifiLoop() {
    if (bleActive && !attemptingConnection && millis() - bleStartTime >= Config::BLE_TIMEOUT_MS) {
        Serial.println("BLE Provisioning timed out.");
        stopBLE();
    }

    if (newCredentialsReceived) {
        newCredentialsReceived = false; 
        attemptingConnection = true;     
        connectionStartTime = millis();  
        bleStartTime = millis();
        
        Serial.println("\n--- Applying new WiFi credentials ---");
        WiFi.disconnect(); 
        delay(500);        
        
        Serial.printf("Connecting to %s...\n", newSSID.c_str());
        WiFi.begin(newSSID.c_str(), newPass.c_str());
    }

    if (attemptingConnection) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Successfully connected!");
            if (bleActive) {
                keysReadByClient = false;
                pStatusChar->setValue("{\"status\":\"connected\"}");
                pStatusChar->notify();
                Serial.println("Notified App: Connected!");

                unsigned long waitStart = millis();
                while (!keysReadByClient && (millis() - waitStart) < 5000) {
                    delay(50);
                }

                delay(300);
                stopBLE();
            }
            attemptingConnection = false; 
        } 
        else if (millis() - connectionStartTime > 15000) {
            Serial.println("Connection Failed: Timeout or Wrong Password.");
            if (bleActive) {
                pStatusChar->setValue("{\"status\":\"failed\",\"reason\":\"Wrong password or weak signal\"}");
                pStatusChar->notify();
                Serial.println("Notified App: Failed!");
            }
            WiFi.disconnect();            
            attemptingConnection = false; 
            bleStartTime = millis();
        }
    }
}