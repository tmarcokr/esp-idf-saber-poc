# ESP32-C6 Lightsaber Proof of Concept

[![ESP-IDF Version](https://img.shields.io/badge/ESP--IDF-v5.4.1-blue)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-orange)](https://en.cppreference.com/w/cpp/20)
[![Target](https://img.shields.io/badge/Target-ESP32--C6-green)](https://www.espressif.com/en/products/socs/esp32-c6)
[![Build Check](https://img.shields.io/github/actions/workflow/status/tmarcokr/esp-idf-saber-poc/build_check.yml?branch=main&label=Build%20Check)](https://github.com/tmarcokr/esp-idf-saber-poc/actions)

Welcome to the **ESP32 Lightsaber Proof of Concept (PoC)**. 
This is a high-performance sandbox built to validate low-latency physics, acoustic reactions, and hardware integrations before applying them to a final, production-ready lightsaber hilt. 

This repository leverages FreeRTOS to handle simultaneous computational loads: polyphonic audio streaming, high-speed gyroscope data fusion, and complex, non-blocking WS2812B LED layer physics, all sharing a single RISC-V core.

---

## 🏗️ Software Architecture & Synchronization

This PoC acts as a central orchestrator that glues together battle-tested components from external repositories. It maintains strict decoupling between *hardware drivers* and *application logic*:

- **`esp-idf-template`**: Provided the robust foundational structure (CMake configuration, `.gitignore`, and the `.agents/` automated CI/CD workflows driven by AI logic).
- **`esp-idf-components`**: Supplies the deeply abstracted, immutable hardware drivers.
  - `AudioEngine`: I2S polyphonic channel mixer.
  - `SmartLed`: Non-blocking, interrupt-safe WS2812B animation engine.
  - `Mpu6050`: I2C DMA-enabled data fusion logic.
- **Synchronization Method**: The core components are synchronized and merged directly into the `components/` tree to ensure architectural immutability. The `main/` orchestrator (`main.cpp`) acts as the conductor, mapping hardware pins, routing motion data to the `SmoothSwingSample` algorithm, and coordinating `BladeIgnite` transitions.

---

## ⚡ Hardware & Electronic Components

The firmware is currently configured for an **ESP32-C6** developer board. Below is the strict pinout mapped within the core orchestrator:

| 🔌 Component | Protocol | Pin Mapping (ESP32-C6) | Description |
| :--- | :--- | :--- | :--- |
| **WS2812B LEDs** | RMT (1-Wire) | `GPIO 0` | Renders dynamic blade animations (ignition, pulsing, hum variations). |
| **Micro SD Card** | SPI | MISO: `4`, MOSI: `11`, SCK: `7`, CS: `10` | Hosts `.WAV` audio files for hums, swings, and lockups. |
| **MAX98357A** | I2S | BCLK: `18`, WS: `19`, DOUT: `20`, SD: `1` | High-fidelity Class-D Amplifier forcing raw digital-to-analog audio output to the speaker. |
| **MPU-6050 IMU** | I2C | SDA: `22`, SCL: `23`, INT: `21` | Advanced 6-axis gyroscope and accelerometer with a built-in Digital Motion Processor (DMP). |
| **BOOT Button** | GPIO | `GPIO 9` | Serves as the primary physical interaction point to trigger the `Ignite()` / `Retract()` state machine. |

> **Tech Note on the MAX98357A**: The amplifier operates on pure I2S logic. The `SD_MODE` pin acts as both shutdown and channel select (Left, Right, or Mono Mix). In this PoC, it receives a steady I2S pump via the FreeRTOS audio task.

---

## 🚀 Quickstart Guide

### 1. Project Configuration
Ensure you have the Espressif ESP-IDF environment loaded in your terminal. We strictly use **v5.4.1**.
```bash
source ~/esp/esp-idf/export.sh
```

### 2. Prepare the SD Card
The project expects the root of the connected SD Card to contain the SmoothSwing audio asset structure. Place your raw `16-bit 44.1kHz` `.WAV` files here:
- `/sdcard/saber/poweron.wav`
- `/sdcard/saber/poweroff.wav`
- `/sdcard/saber/hum.wav`
- `/sdcard/saber/swingL.wav` / `swingH.wav`

### 3. Build & Flash
Set the physical target, compile, output to the board, and attach the real-time serial monitor:
```bash
# Set target architecture
idf.py set-target esp32c6

# Compile and deploy
idf.py build flash monitor
```

### 4. Interactive Testing
Once the serial monitor shows the stabilization log, press the **BOOT Button (GPIO 9)**.
You will hear the `poweron.wav` paired with the physical WS2812B blade extension animation, immediately followed by the responsive `hum` reacting to the MPU-6050 tilt and thrust calculations.
