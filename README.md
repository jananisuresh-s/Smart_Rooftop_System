## 📌 Overview

The Smart Rooftop System is an ESP32-based automated rooftop system designed to protect an open rooftop from rain and high-temperature conditions.

The system monitors environmental conditions using sensors and automatically controls the rooftop mechanism. It also provides **Bluetooth-based manual and voice control through a mobile application**, allowing the user to operate the rooftop remotely using voice commands or manual commands.

## ⚙️ Features

* 🌧️ Automatic rain detection and rooftop closing
* 🌡️ Automatic high-temperature detection
* Temperature and humidity monitoring using DHT11
* Motorized rooftop opening and closing
* Limit-switch-based position control
* 📱 Bluetooth-based mobile control
* 🎙️ Voice-command-based rooftop control through a mobile app
* Partial opening and closing control
* 16×2 I²C LCD status display
* ESP32-based control system

## 📱 Bluetooth & Voice Control

The rooftop can be controlled remotely using a Bluetooth-enabled mobile application.

The user can provide **voice commands through the mobile app**, which are transmitted to the ESP32 over Bluetooth. The ESP32 processes the received command and controls the rooftop motor accordingly.

The system supports commands for:

* Opening the rooftop
* Closing the rooftop
* Stopping movement
* Partial opening
* Partial closing
* Checking system status

### Control Flow

```text
Mobile App
    │
    ├── Voice Command
    │
    ▼
Bluetooth Communication
    │
    ▼
ESP32
    │
    ▼
Command Processing
    │
    ▼
Motor / Relay Control
    │
    ▼
Rooftop Movement
```
