# Setup & Flashing Guide

## 1. Prerequisites

- **Arduino IDE 2.x** (or PlatformIO)
- **ESP32 Board Package** by Espressif Systems (install via *Boards Manager* → search `esp32`)

## 2. Required Libraries

Install these from the Arduino IDE **Library Manager**:

1. `LiquidCrystal I2C` (by Frank de Brabander)
2. `ESP32Servo` (by Kevin Harrington / John K. Bennett)

The remaining libraries (`WiFi`, `WebServer`, `ESPmDNS`, `Wire`, `Preferences`) are built into the ESP32 Arduino core.

## 3. Configure Credentials

1. Copy `firmware/smart_parking/config.example.h` to `firmware/smart_parking/config.h`.
2. Open `config.h` and update:
   - `WIFI_NAME` & `WIFI_PASS` (2.4 GHz Wi-Fi network)
   - `ADMIN_PASS` (password for `/admin`, username is `admin`)
   - `STUDENT_NAME` & `SUPERVISOR` (shown on `/about`)

> `config.h` is listed in `.gitignore` so your credentials are never committed to GitHub.

## 4. Upload to ESP32

1. Open `firmware/smart_parking/smart_parking.ino` in Arduino IDE.
2. Select your ESP32 board (e.g., **ESP32 Dev Module**) and COM port.
3. Click **Upload**.
4. Open **Serial Monitor** at `115200` baud to view the IP address.

## 5. Connecting to the Web App

- **Home Wi-Fi mode**: Open `http://<ESP32-IP>` or `http://danmadinah.local` on a phone/laptop on the same Wi-Fi.
- **Fallback Hotspot mode**: If home Wi-Fi isn't found within 15 seconds, the ESP32 starts a hotspot (`DanMadinah-Lot` / `parking123`). Join it and open `http://192.168.4.1`.
- **Admin panel**: Visit `/admin` and log in with username `admin` and your `ADMIN_PASS`.

## 6. Editing the Web Dashboard

If you edit any HTML, CSS, or JS file inside `web/`, regenerate `firmware/smart_parking/web_pages.h` before uploading:

```bash
python scripts/build_web_header.py
```
