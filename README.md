# ESP32-C6 Lightsaber Proof of Concept

[![ESP-IDF Version](https://img.shields.io/badge/ESP--IDF-v5.4.1-blue)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-orange)](https://en.cppreference.com/w/cpp/20)
[![Target](https://img.shields.io/badge/Target-ESP32--C6-green)](https://www.espressif.com/en/products/socs/esp32-c6)
[![Build Check](https://img.shields.io/github/actions/workflow/status/tmarcokr/esp-idf-saber-poc/build_check.yml?branch=main&label=Build%20Check)](https://github.com/tmarcokr/esp-idf-saber-poc/actions)

Welcome to the **ESP32 Lightsaber Proof of Concept (PoC)**. 
This is a high-performance sandbox built to validate low-latency physics, acoustic reactions, and hardware integrations before applying them to a final, production-ready lightsaber hilt. 

This repository leverages FreeRTOS to handle simultaneous computational loads: polyphonic audio streaming, high-speed gyroscope data fusion, and complex, non-blocking WS2812B LED layer physics, all sharing a single RISC-V core.

---

## 🏗️ Software Design & Layers

The project follows a modular architecture using **Dependency Injection** and **RAII** patterns. The system is divided into clear functional layers to ensure stability and low-latency response:

```text
       ┌─────────────────────────────────────────────────────────┐
       │                   app_main (main.cpp)                   │
       │    (Composition Root & Hardware Configuration)          │
       └──────────────┬─────────────────────────────┬────────────┘
                      │                             │
       ┌──────────────▼──────────────┐      ┌───────▼────────────┐
       │       SaberController       │      │   Internal LED     │
       │     (State Orchestrator)    │      │ (Status Indicator)  │
       └──────────────┬──────────────┘      └────────────────────┘
                      │
      ┌───────────────┼─────────────────────────────┐
      │               │                             │
┌─────▼───────────────▼──────┐      ┌───────────────▼────────────┐
│      SmoothSwingSample     │      │      SmartLed::Engine      │
│  (Motion & Audio Logic)    │      │     (Effect Scheduler)     │
└─────┬───────────────┬──────┘      └───────────────┬────────────┘
      │               │                             │
┌─────▼────────┐┌─────▼────────┐      ┌─────────────┼────────────┐
│ AudioEngine  ││   MPU6050    │      │ BaseEffect  │  Overlays  │
│ (WAV Mixer)  ││ (DMP Sensor) │      │(Ignite/Ret) │(Spark/Blas)│
└─────┬────────┘└─────┬────────┘      └─────────────┴────────────┘
      │               │                             │
      └───────┬───────┴─────────────────────────────┘
              │
    ┌─────────▼────────────────────────────────────────────┐
    │                Hardware Drivers (HAL)                │
    │      (I2S, I2C, RMT/NeoPixel, SDMMC, GPIO)           │
    └──────────────────────────────────────────────────────┘
```

### Core Architecture Components
- **SaberController**: The high-level state machine. It handles user input (button clicks/long-press) and coordinates transitions between the audio and LED layers.
- **SmoothSwingSample**: Contains the physics engine. It processes IMU data (Angular Velocity & Linear Acceleration) to calculate real-time audio volumes and trigger motion-based LED sparks.
- **SmartLed Engine**: A background task that renders multiple effect layers (Base + Overlays) onto the physical LED strip using alpha blending.
- **AudioEngine**: A polyphonic mixer that manages multiple independent audio channels (Hum, Swings, Blaster, etc.) with real-time volume control.

---

## 🎨 Lightsaber Effects

The blade visuals are driven by a composable effect system, allowing for complex animations without blocking the main logic.

| Effect | Type | Description | Trigger |
| :--- | :--- | :--- | :--- |
| **BladeIgnite** | Base | Handles the physical extension/retraction of the light and the steady-state pulse/gradient. | Power On / Off |
| **BladeSpark** | Overlay | Adds random white high-intensity sparkles to the blade. | High-speed motion (Swing) |
| **BlasterImpact** | Overlay | A full-blade red flash that fades out smoothly, simulating a blaster bolt deflection. | Single Click (when ON) |

---

## ⚡ Hardware & Electronic Components

The firmware is currently configured for an **ESP32-C6** developer board. Below is the strict pinout mapped within the core orchestrator:

| 🔌 Component | Protocol | Pin Mapping (ESP32-C6) | Description |
| :--- | :--- | :--- | :--- |
| **Internal LED** | RMT | `GPIO 8` | Signals system readiness (Green) after stabilization. |
| **WS2812B Blade** | RMT | `GPIO 0` | Renders dynamic blade animations. |
| **Micro SD Card** | SPI | MISO: `4`, MOSI: `11`, SCK: `7`, CS: `10` | Hosts `.WAV` audio files. |
| **MAX98357A** | I2S | BCLK: `18`, WS: `19`, DOUT: `20`, SD: `1` | High-fidelity Class-D Amplifier. |
| **MPU-6050 IMU** | I2C | SDA: `22`, SCL: `23`, INT: `21` | Advanced 6-axis gyroscope with DMP. |
| **BOOT Button** | GPIO | `GPIO 9` | Primary interaction point. |

---

## 🚀 Quickstart Guide

### 1. Project Configuration
Ensure you have the Espressif ESP-IDF environment loaded in your terminal. We strictly use **v5.4.1**.
```bash
source ~/esp/esp-idf/export.sh
```

### 2. Prepare the SD Card
Place your raw `16-bit 44.1kHz` `.WAV` files in the following structure:
- `/sdcard/saber/poweron.wav` / `poweroff.wav`
- `/sdcard/saber/hum.wav`
- `/sdcard/saber/swingL.wav` / `swingH.wav`
- `/sdcard/saber/blaster.wav`

### 3. Build & Flash
```bash
idf.py set-target esp32c6
idf.py build flash monitor
```

### 4. Interactive Testing
- **Stabilization**: Wait for the internal LED to turn **Green**.
- **Ignition**: Short press the **BOOT Button**.
- **Blaster Deflection**: Short press the button while the blade is **ON**.
- **Retraction**: Long press the button for **3 seconds**.
- **SmoothSwing**: Move or rotate the hilt to hear the dynamic hum changes and see reactive sparkles.
