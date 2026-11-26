#include "wifi_utils.h"
#include "config.h"
#include "secrets.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CRED_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STATUS_CHAR_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define INFO_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define SCAN_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26ac"

WiFiClient espClient;
bool bleActive = false;

BLECharacteristic *pStatusChar = nullptr;
BLECharacteristic *pInfoChar = nullptr;
BLECharacteristic *pScanChar = nullptr;
volatile bool keysReadByClient = false;
volatile bool scanRequested = false;
bool scanInProgress = false;
String lastScanJson = "[]";
String jsonBuffer = ""; 
bool newCredentialsReceived = false;
String newSSID = "";
String newPass = "";
unsigned long connectionStartTime = 0;
unsigned long bleStartTime = 0;
bool attemptingConnection = false;

struct ScannedNet {
    String ssid;
    int rssi;
    bool locked;
};

static String escapeJsonString(const String& input) {
    String out = "";
    out.reserve(input.length() + 4);
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c == '\"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c >= 0x20) {
            out += c;
        }
    }
    return out;
}

static String buildScanJsonFromResults(int n) {
    if (n <= 0) {
        WiFi.scanDelete();
        return "[]";
    }

    std::vector<ScannedNet> nets;
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        ssid.trim();
        if (ssid.length() == 0) continue;

        int rssi = WiFi.RSSI(i);
        bool locked = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

        bool found = false;
        for (size_t j = 0; j < nets.size(); j++) {
            if (nets[j].ssid == ssid) {
                found = true;
                if (rssi > nets[j].rssi) {
                    nets[j].rssi = rssi;
                    nets[j].locked = locked;
                }
                break;
            }
        }
        if (!found) {
            nets.push_back({ssid, rssi, locked});
        }
    }

    WiFi.scanDelete();

    std::sort(nets.begin(), nets.end(), [](const ScannedNet& a, const ScannedNet& b) {
        return a.rssi > b.rssi;
    });

    String json = "[";
    int added = 0;
    for (size_t i = 0; i < nets.size() && added < 12; i++) {
        String entry = String(added > 0 ? "," : "") +
                       "{\"s\":\"" + escapeJsonString(nets[i].ssid) +
                       "\",\"r\":" + String(nets[i].rssi) +
                       ",\"l\":" + (nets[i].locked ? "true" : "false") + "}";
        if (json.length() + entry.length() + 1 > 480) {
            break;
        }
        json += entry;
        added++;
    }
    json += "]";
    return json;
}

class InfoCharCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) override {
        keysReadByClient = true;
    }
};

class ScanCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue().c_str();
        if (rxValue.length() > 0) {
            scanRequested = true;
        }
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
    if (scanInProgress) {
        WiFi.scanDelete();
        scanInProgress = false;
    }
    scanRequested = false;
    BLEDevice::getAdvertising()->stop();
    BLEDevice::deinit(true);
    bleActive = false;
    Serial.println("BLE stopped and deinitialized.");
}

void startBLE() {
    WiFi.mode(WIFI_STA);
    Serial.println("Running initial WiFi scan before BLE advertising...");
    int n = WiFi.scanNetworks(false, false);
    lastScanJson = buildScanJsonFromResults(n);
    Serial.printf("Initial WiFi scan ready (%d bytes): %s\n", lastScanJson.length(), lastScanJson.c_str());

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

    pScanChar = pService->createCharacteristic(
        SCAN_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    pScanChar->addDescriptor(new BLE2902());
    pScanChar->setValue(lastScanJson.c_str());
    pScanChar->setCallbacks(new ScanCharCallbacks());

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
    scanRequested = false;
    scanInProgress = false;
    jsonBuffer = "";
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(200);
    if (!bleActive) {
        startBLE();
    } else {
        int n = WiFi.scanNetworks(false, false);
        lastScanJson = buildScanJsonFromResults(n);
        if (pScanChar) {
            pScanChar->setValue(lastScanJson.c_str());
        }
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

    if (bleActive && !attemptingConnection) {
        if (scanRequested && !scanInProgress) {
            scanRequested = false;
            scanInProgress = true;
            bleStartTime = millis();
            Serial.println("Starting async WiFi rescan...");
            WiFi.scanNetworks(true, false);
        }

        if (scanInProgress) {
            int n = WiFi.scanComplete();
            if (n >= 0) {
                lastScanJson = buildScanJsonFromResults(n);
                scanInProgress = false;
                Serial.printf("Async WiFi scan complete (%d bytes): %s\n", lastScanJson.length(), lastScanJson.c_str());
                if (pScanChar) {
                    pScanChar->setValue(lastScanJson.c_str());
                    pScanChar->notify();
                }
            } else if (n == WIFI_SCAN_FAILED) {
                Serial.println("Async WiFi scan failed.");
                scanInProgress = false;
                if (pScanChar) {
                    pScanChar->setValue(lastScanJson.c_str());
                    pScanChar->notify();
                }
            }
        }
    }

    if (newCredentialsReceived) {
        newCredentialsReceived = false; 
        if (scanInProgress) {
            WiFi.scanDelete();
            scanInProgress = false;
        }
        scanRequested = false;
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
