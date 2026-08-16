# Setup and Build Guide

This repository contains two independent PlatformIO firmware targets:

- `Edge_Computing32` — ESP32 edge-processing and motor-control node
- `HMI-8266` — ESP8266 local operator interface

## Requirements

- Visual Studio Code + PlatformIO, or PlatformIO Core
- ESP32 development board compatible with `esp32dev`
- ESP8266 board matching the HMI hardware
- 2 × MPU6050-compatible vibration sensors
- TB6612FNG dual motor driver
- 2 × DC motors
- Local Wi-Fi network for dashboard/HMI communication

## Local credentials

Real Wi-Fi credentials are intentionally excluded from version control.

Create local secret files from the tracked templates:

```bash
cp Edge_Computing32/src/secrets.example.h Edge_Computing32/src/secrets.h
cp HMI-8266/src/secrets.example.h HMI-8266/src/secrets.h
```

Edit the generated files with the network used by the prototype. Both `secrets.h` files are ignored by Git.

## ESP32 edge node

### Build

```bash
cd Edge_Computing32
pio run
```

### Upload firmware

Connect the ESP32 and run:

```bash
pio run --target upload
```

### Upload dashboard assets

The dashboard assets are stored in `Edge_Computing32/data/` and must be uploaded to the SPIFFS partition:

```bash
pio run --target uploadfs
```

The custom `partitions_no_ota.csv` layout allocates additional flash space to the dashboard resources.

### Serial monitor

```bash
pio device monitor
```

Default serial monitor speed: `115200` baud.

## ESP8266 HMI

### Build

```bash
cd HMI-8266
pio run
```

### Upload

```bash
pio run --target upload
```

### Serial monitor

```bash
pio device monitor
```

## Default signal-processing configuration

The ESP32 configuration currently uses:

| Parameter | Value |
|---|---:|
| Sampling frequency | 400 Hz |
| FFT frame size | 256 samples |
| RMS warning threshold | 1.50 |
| RMS critical threshold | 3.00 |
| Bearing-analysis band | 100–180 Hz |

These values belong to the current experimental prototype and should be recalibrated for a different motor, mounting structure, sensor placement or industrial installation.

## I2C topology

### ESP32 vibration acquisition

| Bus | SDA | SCL | Speed | Purpose |
|---|---:|---:|---:|---|
| I2C 0 | GPIO 21 | GPIO 22 | 400 kHz | MPU6050 #1 |
| I2C 1 | GPIO 18 | GPIO 19 | 100 kHz | MPU6050 #2 |

The dual-bus layout was selected to isolate two similar sensors without requiring an external multiplexer.

### ESP8266 HMI

The HMI shares its I2C bus between the SSD1306 OLED and PCF8574 I/O expander. The PCF8574 is configured at address `0x20`.

## Motor driver

The ESP32 controls a TB6612FNG driver using 20 kHz PWM with 8-bit duty-cycle control.

Current pin mapping:

| Signal | ESP32 GPIO |
|---|---:|
| STBY | 27 |
| PWMA | 33 |
| AIN1 | 26 |
| AIN2 | 25 |
| PWMB | 32 |
| BIN1 | 14 |
| BIN2 | 13 |

## Network paths

The prototype uses several network mechanisms:

- UDP port `3333` for HMI discovery/commands/telemetry
- MQTT for secondary telemetry
- WebSocket for dashboard updates
- HTTP for the self-hosted dashboard

The MQTT broker address is currently configured in `Edge_Computing32/src/config.h` and may need to be changed for another network.

## Calibration notes

The vibration-processing code contains sensor offset compensation derived from the physical sensors used during development. If the MPU6050 modules are replaced, mounted differently or used on a different structure, recalibration is recommended before interpreting the health indicators.

## Safety and scope

This is an educational/experimental embedded prototype. The health score is a threshold-derived condition indicator rather than a certified safety metric or Remaining Useful Life prediction.

Do not use the prototype as the sole protection mechanism for production machinery without appropriate industrial validation, electrical protection, fail-safe design and applicable safety controls.
