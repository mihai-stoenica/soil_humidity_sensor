# ESP32 Soil Humidity & Plant Monitor

An ESP32-based plant monitoring and automated watering controller. It reads environmental sensors, reports telemetry over MQTT, and controls a water pump.
[Hardware part design](https://github.com/mihai-stoenica/soil_humidity_pcb.git)

## Features

- **Sensor Monitoring:** Reads soil moisture, ambient temperature & humidity (DHT sensor), and light levels.
- **Pump Control:** Drives a water pump relay via GPIO for remote or automated watering.
- **MQTT & Live Telemetry:** Publishes periodic database records and live updates over Wi-Fi/MQTT.
- **Status Indicator:** Onboard status LED indicates Wi-Fi and MQTT connection states.

## Hardware & Pinout

Configured in `include/config.h`:

| Component | ESP32 GPIO Pin |
| :--- | :--- |
| Soil Humidity Sensor | `GPIO 34` |
| Light Sensor | `GPIO 32` |
| DHT Sensor (Temp/Humidity) | `GPIO 5` |
| Water Pump Relay | `GPIO 33` |
| Status LED | `GPIO 13` |

## Getting Started

1. **Configure Credentials:** Create `include/secrets.h` with your Wi-Fi and MQTT credentials.
2. **Build & Upload:** Open the project in VS Code with the PlatformIO extension and run **Upload**, or via CLI:
   ```bash
   pio run --target upload
   pio device monitor
   ```
