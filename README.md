# Driver Attention Monitoring System

## Hardware Components
- ESP32 microcontroller
- NeuroSky Mindwave EEG headset
- 0.96" SSD1306 OLED display (128x64)
- WS2812B LED strip (6 LEDs)
- Piezo buzzer

## Pin Connections
| Component       | ESP32 Pin |
|-----------------|-----------|
| OLED SDA        | GPIO8     |
| OLED SCL        | GPIO9     |
| LED Strip Data  | GPIO48    |
| Buzzer          | GPIO45    |
| Mindwave RX     | GPIO6     |
| Mindwave TX     | GPIO5     |

## Features
- Real-time drowsiness detection using EEG data
- Attention/meditation level monitoring
- Visual (LED) and auditory (buzzer) feedback
- OLED display for status information
- Four detection states: Alert/Drowsy/Attention Lost/Distracted

## Libraries Required
```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include "Mindwave.h"
