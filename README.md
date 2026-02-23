
<p align="center">
  <img src="assets/banner.png" width="100%">
</p>

# 🛰️ OrbitalEye — ISS Tracker (ESP32)

Real-time ISS tracker built with ESP32 and OLED display.  
OrbitalEye renders a rotating globe, tracks the International Space Station live and displays orbital and crew data.

---

## 🚀 Preview

<p align="center">
  <img src="assets/preview.png" width="700">
</p>

---

## ✨ Features

- Real-time ISS position
- Software rendered rotating globe on OLED
- Orbital data display
- Automatic location detection (country / ocean)
- Astronauts currently aboard ISS
- Multi-screen UI
- Live API integration

---

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

---

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
