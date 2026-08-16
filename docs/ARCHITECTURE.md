# System Architecture

## Overview

The project is organized as a distributed embedded monitoring platform with three functional layers:

1. **ESP32 edge-processing node** — vibration acquisition, DSP, motor control, networking.
2. **ESP8266 local HMI** — operator feedback, navigation, commands, alerts.
3. **Browser / central monitoring layer** — live dashboard and secondary telemetry.

## ESP32 Edge Node

The ESP32-WROOM-32 is used as the main compute platform. Its dual-core architecture is exploited to separate deterministic sensing and DSP from Wi-Fi/network processing.

### Core allocation

| Core | Task | Main responsibilities |
|---|---|---|
| Core 1 | `SensTask` | MPU6050 acquisition, frame construction, FFT, condition metrics |
| Core 0 | `NetTask` | UDP, MQTT, WebSocket, HTTP/network services |

The sensor task runs with the higher priority. Shared metrics and FFT buffers are protected with FreeRTOS mutexes before being accessed by the network task.

## Sampling and DSP

The documented configuration uses:

- Sampling frequency: **400 Hz**
- Frame length: **256 samples**
- Approximate spectral range of interest: **0–200 Hz**

The signal-processing flow is:

1. Read 3-axis acceleration from the MPU6050.
2. Apply calibrated sensor offsets.
3. Compute acceleration-vector magnitude.
4. Remove the static gravity component.
5. Remove the frame mean to suppress the DC component.
6. Compute time-domain metrics.
7. Apply a Hamming window.
8. Execute the FFT.
9. Suppress very-low-frequency content.
10. Extract spectral metrics and publish the resulting condition indicators.

### Extracted features

- RMS vibration
- Peak amplitude
- Crest factor
- Dominant frequency
- Relative energy in the 100–180 Hz bearing-analysis band
- RMS-derived health indicator

## Dual-Bus I2C Acquisition

The two vibration sensors are isolated on different ESP32 hardware I2C controllers:

| Sensor path | SDA | SCL | Bus speed |
|---|---:|---:|---:|
| I2C Bus 0 | GPIO 21 | GPIO 22 | 400 kHz |
| I2C Bus 1 | GPIO 18 | GPIO 19 | 100 kHz |

This design was selected to avoid address conflicts between similar MPU6050 devices without requiring an external I2C multiplexer.

A non-standard MPU6050-compatible unit encountered during prototyping reported an atypical silicon identifier. The initialization workflow and underlying library handling were adapted to support the device used in the experimental setup.

## Motor Control

Two DC motors are driven through a TB6612FNG dual H-bridge.

- PWM frequency: **20 kHz**
- PWM resolution: **8 bit**
- Independent direction control for both motors
- Standby path used as part of the shutdown / emergency-stop logic

The ultrasonic PWM frequency avoids the audible switching noise associated with lower PWM frequencies.

## Local HMI

The local console uses an ESP8266-based IdeaSpark board with an integrated SSD1306 OLED.

Additional I/O is provided through a PCF8574 expander sharing the I2C bus with the display. A 4×4 matrix keypad provides local navigation and command entry, while a buzzer provides local warning/critical feedback.

The HMI software is split into modules for:

- display rendering
- input handling
- networking
- settings
- application state

## Communication Architecture

### UDP

Used for the low-latency path between the ESP32 and ESP8266 HMI. The documented prototype includes local subnet discovery using discovery request/response packets followed by JSON telemetry and commands.

### MQTT

Used as a secondary telemetry path toward a central monitoring station / CLI and as a basis for later database or cloud integration.

### WebSocket

Used for live browser telemetry, including motor metrics and FFT data.

### HTTP

The ESP32 serves the dashboard itself from SPIFFS. Static web resources are cached client-side to reduce repeated transfers and network-core load.

## Web Dashboard

The dashboard is implemented as a Single Page Application stored in the ESP32 flash filesystem. It provides live visualization of condition metrics and spectra and offers control functions without depending on an external cloud service.

## Power and Signal Integrity

Because the prototype combines inductive motor loads and vibration sensors on the same system, the hardware includes measures intended to reduce resets and communication corruption:

- local logic decoupling around the motor-driver supply
- bulk capacitance on the motor-power rail
- stable I2C pull-up configuration
- separate I2C controllers for the two sensor paths

## Design Intent

The architecture follows an edge-computing principle: raw high-rate vibration data is processed locally, while compact features and spectra are distributed to the operator interfaces. This reduces network dependency and demonstrates how machine-condition analysis can be performed directly near the monitored asset.
