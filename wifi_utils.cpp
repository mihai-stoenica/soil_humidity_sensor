#include "wifi_utils.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CRED_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STATUS_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26aa"

WiFiClient espClient;
bool bleActive = false;

BLECharacteristic *pStatusChar;
String jsonBuffer = ""; 
bool newCredentialsReceived = false;
String newSSID = "";
String newPass = "";
unsigned long connectionStartTime = 0;
bool attemptingConnection = false;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
        
        if (rxValue.length() > 0) {
            for (int i = 0; i < rxValue.length(); i++) {
                if (rxValue[i] != '\0') { 
                    jsonBuffer += rxValue[i];
                }
            }

            if (jsonBuffer.indexOf('}') != -1) { 
                Serial.println("Full JSON Received: " + jsonBuffer);

                DynamicJsonDocument doc(512); 
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

void startBLE() {
    BLEDevice::init("PROV_MyESP32");
    BLEServer *pServer = BLEDevice::createServer();
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    BLECharacteristic *pCredChar = pService->createCharacteristic(CRED_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pCredChar->setCallbacks(new MyCallbacks());

    pStatusChar = pService->createCharacteristic(STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("BLE Provisioning Active...");
    bleActive = true;
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
    if (newCredentialsReceived) {
        newCredentialsReceived = false; 
        attemptingConnection = true;     
        connectionStartTime = millis();  
        
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
                pStatusChar->setValue("{\"status\":\"connected\"}");
                pStatusChar->notify();
                Serial.println("Notified App: Connected!");
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
        }
    }
}