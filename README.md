# Embedded Predictive Maintenance System for DC Motors

An embedded **Industry 4.0 condition-monitoring and predictive-maintenance prototype** for detecting mechanical anomalies in DC motors through real-time vibration analysis at the edge.

The project combines an **ESP32 dual-core edge node**, two **MPU6050 vibration sensors**, FFT-based digital signal processing, a local **ESP8266 HMI**, a self-hosted browser dashboard, motor control, and hybrid UDP/MQTT communication.

> **Scope note:** the current prototype performs real-time condition monitoring, anomaly detection and health-indicator extraction. It does **not** estimate Remaining Useful Life (RUL) or predict an absolute failure time.

## What this project demonstrates

- Embedded vibration monitoring for **two DC motors**
- Deterministic acquisition at **400 Hz** with **256-sample FFT frames**
- RMS, peak, crest factor, dominant-frequency, bearing-band and health indicators
- Hamming windowing and low-frequency/DC suppression before spectral analysis
- **FreeRTOS dual-core task partitioning** on ESP32
- Two independent hardware I2C buses for identical sensors
- **UDP + MQTT + WebSocket** communications
- Local ESP8266 HMI with OLED, keypad and audible alerts
- **20 kHz PWM** motor control through a TB6612FNG driver
- A cloud-independent dashboard hosted directly from ESP32 flash
- Experimental validation using nominal and mechanically faulted motor states

## System architecture

```text
                         ┌─────────────────────────────┐
                         │         Web Browser         │
                         │ Dashboard / FFT / Controls  │
                         └──────────────▲──────────────┘
                                        │ HTTP / WebSocket
                                        │
┌─────────────────┐       ┌─────────────┴───────────────┐       ┌─────────────────┐
│   MPU6050 #1    │ I2C-0 │          ESP32 Edge         │ I2C-1 │   MPU6050 #2    │
│ Motor 1 sensor  ├──────►│ Core 1: acquisition + DSP   │◄──────┤ Motor 2 sensor  │
└─────────────────┘       │ Core 0: network services    │       └─────────────────┘
                          └──────────┬─────────┬─────────┘
                                     │         │
                                  UDP│         │MQTT
                                     │         ▼
                          ┌───────────▼────┐  Central telemetry / CLI
                          │ ESP8266 HMI    │
                          │ OLED + keypad  │
                          │ buzzer         │
                          └────────────────┘
                                     │
                                     │ operator commands
                                     ▼
                          ┌────────────────────┐
                          │ TB6612FNG Driver   │
                          │ 2 × DC motors      │
                          └────────────────────┘
```

For a deeper breakdown, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Edge-processing pipeline

```text
Vibration
   │
   ▼
MPU6050 acceleration acquisition
   │
   ▼
Sensor offset compensation
   │
   ▼
Acceleration magnitude / gravity removal
   │
   ▼
Mean removal
   │
   ├──────────────► RMS / Peak / Crest Factor
   │
   ▼
Hamming window
   │
   ▼
FFT
   │
   ├──────────────► Dominant frequency
   │
   ├──────────────► Spectral energy (100–180 Hz)
   │                    │
   │                    ▼
   │              Bearing indicator
   │
   └──────────────► Health indicator
                        │
                        ▼
                UDP / MQTT / WebSocket
```

## Real-time software architecture

The ESP32 firmware separates time-critical acquisition from communication using FreeRTOS.

### Core 1 — `SensTask`

- Acquires both vibration channels
- Builds sample frames
- Calculates FFT and condition metrics
- Updates shared metric and spectrum buffers
- Runs at a higher task priority than networking

### Core 0 — `NetTask`

- Handles Wi-Fi and network services
- Publishes metrics and FFT data
- Feeds the dashboard at approximately 5 Hz
- Handles UDP, MQTT and WebSocket traffic

Shared metrics and FFT buffers are protected by FreeRTOS mutexes to avoid race conditions between tasks.

## Condition indicators

| Metric | Purpose |
|---|---|
| RMS | Overall vibration severity |
| Peak | Maximum vibration excursion |
| Crest factor | Highlights impulsive mechanical events |
| Dominant frequency | Strongest spectral component |
| Bearing indicator | Relative energy in the configured 100–180 Hz band |
| Health indicator | Normalized RMS-based engineering indicator from 0–100% |

The health value is a **heuristic engineering indicator** derived from measured RMS relative to a configured critical threshold. It is not a probabilistic failure prediction.

## Hardware

### ESP32 edge node

- ESP32-WROOM-32
- 2 × MPU6050 accelerometer/gyroscope modules
- TB6612FNG dual motor driver
- 2 × DC motors
- Local decoupling and bulk filtering for motor-induced supply disturbances

### ESP8266 local HMI

- ESP8266 / IdeaSpark board
- Integrated SSD1306 OLED
- PCF8574 I/O expander
- 4×4 matrix keypad
- Buzzer for warning/critical alerts

## Key engineering decisions

### Dual-bus I2C acquisition

The two vibration sensors are isolated on separate ESP32 hardware I2C buses:

- **Bus 0:** SDA 21 / SCL 22 at 400 kHz
- **Bus 1:** SDA 18 / SCL 19 at 100 kHz

This avoids an external I2C multiplexer while allowing simultaneous use of two similar sensors. During integration, a non-standard MPU6050-compatible device with an atypical silicon identifier was also handled by adapting the initialization path.

### Motor drive and EMC mitigation

The motors are driven by a TB6612FNG using **20 kHz, 8-bit PWM**. The physical prototype uses local decoupling and bulk capacitance to reduce power-rail disturbances caused by inductive loads and startup current.

## Communications

Different communication paths are used for different system requirements:

- **UDP** — low-latency discovery, HMI telemetry and commands
- **MQTT** — secondary telemetry path for a central monitoring station
- **WebSocket** — live dashboard updates
- **HTTP** — self-hosted SPA resources from ESP32 flash
- **JSON** — telemetry and command serialization

The ESP8266 HMI performs local network discovery and communicates asynchronously with the ESP32 edge node.

## Web dashboard

The ESP32 hosts a Single Page Application directly from SPIFFS, providing:

- Live motor condition metrics
- FFT/spectral visualization
- Motor state information
- Operator controls
- Browser-side caching to reduce repeated flash/network load

The dashboard does not require an external cloud service.

## Experimental validation

The prototype was validated with mechanically coupled DC motor assemblies under baseline, nominal and induced-fault conditions.

The documented tests include:

- Noise-floor characterization
- Healthy vs. mechanically faulted operation
- Induced rotor imbalance
- Mechanical crosstalk across a shared structure
- Spectral fault-signature extraction
- Health-indicator response
- Network latency measurements
- Dashboard resource-loading and caching measurements

A fault-related spectral component around **43.8 Hz** was identified in the documented imbalance experiment. The low-latency control path achieved an average reaction time below **20 ms** in the reported test setup.

The dashboard validation also recorded **645 ms** for a cold resource load and **152 ms** for a cached reload.

See [`docs/VALIDATION.md`](docs/VALIDATION.md) for the experimental summary.

## Repository structure

```text
.
├── README.md
├── .gitignore
├── docs/
│   ├── ARCHITECTURE.md
│   ├── SETUP.md
│   └── VALIDATION.md
│
├── Edge_Computing32/        # ESP32 edge-processing firmware
│   ├── data/                # Web dashboard: HTML / CSS / JavaScript
│   ├── src/                 # Sensors, DSP, motors, networking
│   │   └── secrets.example.h
│   ├── platformio.ini
│   └── partitions_no_ota.csv
│
└── HMI-8266/                # ESP8266 HMI firmware
    ├── src/                 # Display, input, networking, state
    │   └── secrets.example.h
    └── platformio.ini
```

## Toolchain and libraries

- C++17
- PlatformIO
- Arduino framework
- FreeRTOS
- ArduinoFFT
- ArduinoJson
- Adafruit MPU6050
- AsyncTCP
- ESPAsyncWebServer
- PubSubClient / MQTT
- U8g2
- SPIFFS

## Quick start

### 1. Clone

```bash
git clone https://github.com/onibrute/ASI-project.git
cd ASI-project
```

### 2. Create local credential files

The repository does not require Wi-Fi credentials to be committed.

```bash
cp Edge_Computing32/src/secrets.example.h Edge_Computing32/src/secrets.h
cp HMI-8266/src/secrets.example.h HMI-8266/src/secrets.h
```

Edit both `secrets.h` files with your local network values. They are ignored by Git.

### 3. Build ESP32 edge firmware

```bash
cd Edge_Computing32
pio run
```

### 4. Build ESP8266 HMI firmware

```bash
cd ../HMI-8266
pio run
```

See [`docs/SETUP.md`](docs/SETUP.md) for flashing, web assets and configuration notes.

## Project status

### Implemented

- [x] Dual vibration sensing
- [x] Real-time FFT processing
- [x] RMS / peak / crest-factor extraction
- [x] Dominant-frequency extraction
- [x] Bearing-band energy indicator
- [x] RMS-based health indicator
- [x] Dual-core ESP32 task architecture
- [x] UDP HMI communication
- [x] MQTT telemetry path
- [x] WebSocket dashboard telemetry
- [x] Local ESP8266 HMI
- [x] PWM motor control
- [x] Experimental fault-injection tests

### Planned / research directions

- [ ] Long-term condition history and trend analysis
- [ ] Dataset collection across multiple fault classes
- [ ] TinyML anomaly detection on the edge
- [ ] NTP timestamp synchronization
- [ ] TLS for MQTT/WebSocket communications
- [ ] Automated threshold calibration
- [ ] Remaining Useful Life estimation

## Why this project matters

The project explores how low-cost embedded hardware can move machine-condition analysis closer to the physical asset. Rather than continuously streaming raw high-rate vibration data to a remote service, the edge node extracts compact diagnostic features locally and distributes the information required by operators and higher-level systems.

It combines **embedded systems, industrial communication, digital signal processing, motor control, IIoT, real-time software and condition monitoring** in a single working prototype.

## Documentation

The repository documentation is derived from the implemented firmware and the associated technical project report for the Embedded Systems Architecture course.

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — system and software architecture
- [`docs/VALIDATION.md`](docs/VALIDATION.md) — experimental methodology and measured results
- [`docs/SETUP.md`](docs/SETUP.md) — local configuration, build and flash workflow

## Author

**Robert Constantin Preda**  
Master's programme: Information Technologies in Systems Engineering  
Project developed for the Embedded Systems Architecture course.
