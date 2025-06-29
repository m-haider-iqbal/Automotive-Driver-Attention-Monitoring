# Driver Attention System

![System Diagram](system_diagram.png) *Example system diagram (replace with actual image)*

## Table of Contents
- [Overview](#overview)
- [Hardware Components](#hardware-components)
- [Software Architecture](#software-architecture)
- [State Machine Logic](#state-machine-logic)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
- [API Reference](#api-reference)
- [License](#license)

## Overview

The Driver Attention System is an IoT device that monitors driver alertness using EEG signals from a NeuroSky Mindwave headset. The system provides visual and auditory feedback when drowsiness or distraction is detected.

Key Features:
- Real-time drowsiness detection
- Multi-modal feedback (LEDs, buzzer, OLED display)
- State-based alert system
- Historical data tracking

## Hardware Components

| Component | Specification | Connection |
|-----------|---------------|------------|
| ESP32 MCU | Dual-core 240MHz | - |
| SSD1306 OLED | 128x64 pixels | I2C (SDA=8, SCL=9) |
| WS2812B LED Strip | 6 LEDs | GPIO 48 |
| Buzzer | Passive | GPIO 45 |
| NeuroSky Mindwave | UART | RX=6, TX=5 |

![Wiring Diagram](wiring_diagram.png) *Example wiring diagram*

## Software Architecture

### Main Components
1. **Mindwave EEG Processor** - Handles brainwave data acquisition
2. **State Manager** - Tracks driver state with hysteresis
3. **Feedback System** - Controls LEDs, buzzer and display
4. **History Buffer** - Maintains rolling window of metrics

### Key Libraries
- `Adafruit_SSD1306` - OLED display control
- `FastLED` - Addressable LED control
- `Mindwave` - NeuroSky headset communication

## State Machine Logic

```mermaid
stateDiagram-v2
    [*] --> ALERT
    ALERT --> DROWSY: Score > 0.55
    DROWSY --> ALERT: Score < 0.25
    DROWSY --> ATTENTION_LOST: Score > 0.85
    ATTENTION_LOST --> DROWSY: Score < 0.65
