#   Phantom ESP32
*A lightweight open-source bicycle navigation computer built with the ESP32.*

BikeNav ESP32 is a DIY GPS cycling computer designed for outdoor use. It combines an **ESP32**, **u-blox MAX-M10M GPS receiver**, **Sharp Memory LCD**, and **microSD card** to create a low-power navigation device capable of displaying maps, tracking rides, and providing real-time cycling metrics.

The Sharp Memory LCD provides excellent sunlight readability while consuming very little power, making it ideal for long rides.

---

## Features

- 🗺️ Offline map rendering from microSD card
- 📍 Real-time GPS positioning using the u-blox MAX-M10M
- 🚴 Live cycling data
  - Current speed
  - Average speed
  - Trip distance
  - Total distance
  - Heading
  - Ride time
- 🧭 Turn-by-turn capable map interface (planned)
- 💾 GPX ride logging to SD card
- 🌞 Sunlight-readable Sharp Memory LCD
- 🔋 Low power design for extended battery life
- 📡 Fast satellite acquisition with multi-band GNSS support
- Bluetooth capabilities for uploading to fitness apps such as Strava

---

## Hardware

| Component | Description |
|------------|-------------|
| ESP32 | Main processor |
| MAX-M10M GPS | High-performance GNSS receiver |
| Sharp Memory LCD | Outdoor-readable display |
| microSD Card Module | Offline maps and GPX storage |
| Li-Ion/LiPo Battery | Portable power |
| Battery Charger | USB charging circuit (optional) |
| Handlebar Mount | Bicycle mounting system |

## Offline Maps

Maps are stored on the microSD card and loaded as needed.

Possible formats include:

- Raster map tiles
- Vector tiles (future)
- Custom compressed tile database

The map renderer centers on the current GPS position while displaying nearby roads and paths.

---

## Ride Data

The computer continuously records:

- Current speed
- Average speed
- Maximum speed
- Total distance
- Ride duration
- GPS track
- Timestamp
- Latitude
- Longitude

Tracks are stored as GPX files for later upload to services such as Strava or Garmin Connect.

---

## Why Sharp Memory LCD?

Unlike TFT or OLED displays, the Sharp Memory LCD offers:

- Excellent readability in direct sunlight
- Extremely low power consumption
- No backlight required during daytime
- Fast refresh for navigation
- Long battery life

These characteristics make it particularly well-suited for bicycle computers and outdoor GPS devices.

---
