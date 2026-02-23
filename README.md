<p align="center">
  <img src="assets/banner.png" width="100%">
</p>


<p align="center">

<img src="https://img.shields.io/badge/ESP32-Project-blue">
<img src="https://img.shields.io/badge/PlatformIO-ready-orange">
<img src="https://img.shields.io/badge/status-active-success">
<img src="https://img.shields.io/badge/license-MIT-green">

</p>
# 🛰️ OrbitalEye — ISS Tracker (ESP32)

Real-time ISS tracker built with ESP32 and OLED display.  
OrbitalEye renders a rotating globe, tracks the International Space Station live and displays orbital and crew data. 


## ✨ Features

- Real-time ISS position
- Software rendered rotating globe on OLED
- Orbital data display
- Automatic location detection (country / ocean)
- Astronauts currently aboard ISS
- Multi-screen UI
- Live API integration
## 🎥 Demo
<p align="center">
  <img src="assets/ISS.gif" width="700">
</p>

## 🧰 Hardware

- ESP32 (DevKit / WROOM)
- SSD1306 OLED 128x64 (I2C)

---

## 🧠 Tech Stack

- PlatformIO
- Arduino framework
- WiFi networking
- HTTP APIs
- ArduinoJson
- Custom globe renderer

---

## 📡 APIs

- http://api.open-notify.org/iss-now.json
- http://api.open-notify.org/astros.json

---

## ⚙️ Getting Started

1. Clone the repository
2. Open with PlatformIO
3. Add WiFi credentials
4. Upload to ESP32

## 🚀 Version

OrbitalEye v1 — Initial release  
Software globe renderer + live ISS tracking

## 🗺️ Roadmap

- Buttons navigation
- UI animations
- Multi-satellite tracking
- Offline orbit prediction
- CYD version
- Color display version
- Space weather data

---

## 📜 License

MIT
