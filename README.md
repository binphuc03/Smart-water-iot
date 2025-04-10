# 🌱 Smart Irrigation System using ESP32 & Firebase

## 📌 Overview

This project is a smart irrigation system built with ESP32, Firebase, and several environmental sensors. It allows plants to be watered automatically based on soil moisture, or manually through Firebase control. Users can switch between 3 irrigation modes: automatic, manual, and scheduled.

---

## 🛠️ Hardware Components

- ESP32 DevKit
- DHT11 sensor (temperature & humidity)
- Soil Moisture Sensor (analog)
- BH1750 light sensor (I2C)
- Relay module
- LCD 16x2 with I2C
- Mini water pump or solenoid valve
- Power supply (5V/12V)

---

## 📂 Folder Structure

```
SmartIrrigation/
├── SmartIrrigation.ino
├── README.md
└── /libraries (optional)
```

---

## ⚙️ Required Libraries

Install these libraries in Arduino IDE:

- `DHT sensor library by Adafruit`
- `BH1750 by claws`
- `LiquidCrystal_I2C`
- `WiFiManager`
- `Firebase ESP Client by mobizt`
- `NTPClient`
- `TimeLib`

---

## 🔌 Wiring Diagram

| Component         | ESP32 Pin         |
|------------------|-------------------|
| DHT11            | D2                |
| Soil Moisture    | A3 (GPIO35)       |
| Relay            | D33               |
| LCD I2C          | SDA: D21, SCL: D22|
| BH1750           | SDA: D21, SCL: D22|

---

## ☁️ Firebase Setup

1. Go to [Firebase Console](https://console.firebase.google.com) and create a project.
2. Enable **Realtime Database** and select **Start in test mode**.
3. Retrieve:
   - **Database URL**
   - **Database Secret** (Legacy credentials)
   - **API Key** (from Project Settings)

Update your Arduino code:
```cpp
#define DATABASE_URL "https://<your-project>.firebaseio.com"
#define DATABASE_SECRET "your_firebase_database_secret"
#define API_KEY "your_firebase_api_key"
```

---

## 📲 WiFiManager Setup

If the ESP32 has no saved Wi-Fi, it will create a temporary access point:
- SSID: `ESP32-WiFi-Manager`
- Password: `123456789`

Connect and open browser to `192.168.4.1` to configure Wi-Fi.

---

## ▶️ How to Run

1. Connect hardware as per wiring diagram.
2. Open `SmartIrrigation.ino` and upload to ESP32.
3. Configure Wi-Fi through the captive portal.
4. Monitor Firebase at path `/plants/0/` for real-time data.

---

## 📊 Firebase Data Structure

Firebase stores sensor readings and control flags:

```
/plants/0/
├── humidity
├── temperature
├── moisture
├── light
├── water_mode             // 1: auto, 2: manual, 3: schedule
├── manual_mode
│   ├── server             // controlled from server
│   └── device             // device execution status
├── automatic_mode
│   └── duration
├── low_threshold
├── high_threshold
├── max_duration
├── updated_log_time
├── daylogs/               // hourly logs
└── device_mac
```

---

## 🔁 Irrigation Modes

- **Mode 1 (Automatic):**
  Automatically irrigates based on soil moisture levels.
- **Mode 2 (Manual):**
  Manual control through Firebase (`manual_mode/server` flag).
- **Mode 3 (Schedule):**
  Irrigation based on scheduled duration (future development).

---

## ⏱️ Irrigation Timing Formula

```cpp
T = max_duration * (threshold - soilMoisture) / threshold;
```

This ensures that the more the soil is dry, the longer the irrigation.

---

## 💡 Features

- Dynamic Wi-Fi setup using WiFiManager
- Real-time sensor logging
- Manual or automatic irrigation
- Firebase Realtime Database integration
- LCD display feedback
- Hourly logging for analytics

---

## 🧠 Technical Notes

- Only sends updates to Firebase every 5 minutes if values change.
- Reconnects to Wi-Fi/Firebase on disconnection.
- Uses `millis()` instead of `delay()` to maintain responsiveness.

---

## 🧪 Known Challenges & Solutions

| Challenge                       | Solution                                                  |
|--------------------------------|------------------------------------------------------------|
| Firebase or Wi-Fi disconnection| Auto reconnect or reset on failure                        |
| Unstable DHT11 readings        | Retry if `NaN`; consider upgrading to DHT22               |
| Static Wi-Fi configuration     | Used WiFiManager for user-friendly setup                  |
| RAM usage on ESP32             | Monitor via `ESP.getFreeHeap()` and optimize libraries    |

---

## 📸 Screenshots & Diagrams

(https://github.com/binphuc03/Smart-water-iot/blob/main/design_watering_system.png)

---

## 📃 License

MIT License (or your preferred license)

---

## 🙋‍♂️ Authors

- [Your Name] – Hardware & Firmware
- [Teammate Name] – Firebase Integration & UI

---

Feel free to contribute or raise issues if you'd like to improve this system!
