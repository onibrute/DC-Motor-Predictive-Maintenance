# Embedded Predictive Maintenance System for DC Motors

An embedded **Industry 4.0 condition-monitoring and predictive-maintenance prototype** for detecting mechanical anomalies in DC motors through real-time vibration analysis at the edge.

The system combines an **ESP32 dual-core edge node**, two **MPU6050 vibration sensors**, FFT-based digital signal processing, a local **ESP8266 HMI**, a browser-based dashboard, motor control, and hybrid UDP/MQTT communication.

> The current prototype performs real-time condition monitoring, anomaly detection, and health-indicator extraction. It does **not** estimate Remaining Useful Life (RUL) or predict an absolute failure time.

## Highlights

- Dual-motor vibration monitoring with two MPU6050 sensors
- Deterministic sampling at **400 Hz**
- **256-sample** FFT processing windows
- RMS, peak, crest factor, dominant-frequency, bearing-band, and health indicators
- Hamming windowing and low-frequency/DC suppression before spectral analysis
- FreeRTOS task pinning across both ESP32 cores
- Dual hardware I2C buses for identical sensors without an external multiplexer
- Hybrid **UDP + MQTT + WebSocket** communications
- Local ESP8266 HMI with OLED, keypad, and audible warnings
- 20 kHz PWM motor control through a TB6612FNG driver
- Self-hosted web dashboard stored in SPIFFS
- Experimental validation using healthy and mechanically faulted DC motors

## System Architecture

```text
                         ┌─────────────────────────────┐
                         │         Web Browser         │
                         │ Dashboard / FFT / Controls  │
                         └──────────────▲──────────────┘
                                        │ WebSocket / HTTP
                                        │
┌─────────────────┐       ┌─────────────┴───────────────┐       ┌─────────────────┐
│   MPU6050 #1    │ I2C-0 │          ESP32 Edge         │ I2C-1 │   MPU6050 #2    │
│ Motor 1 sensor  ├──────►│  Core 1: acquisition + DSP  │◄──────┤ Motor 2 sensor  │
└─────────────────┘       │  Core 0: network services   │       └─────────────────┘
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

## Edge Processing Pipeline

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

## Embedded Software Architecture

The ESP32 application uses FreeRTOS to separate time-critical signal acquisition from networking:

### Core 1 — `SensTask`

- Reads both MPU6050 sensors
- Builds vibration frames
- Calculates FFT and condition metrics
- Updates shared metric and spectrum buffers
- Runs at higher priority than the network task

### Core 0 — `NetTask`

- Handles Wi-Fi/network services
- Publishes metrics and FFT data
- Updates the web dashboard at approximately 5 Hz
- Handles UDP, MQTT and WebSocket traffic

Shared metrics and FFT buffers are protected by FreeRTOS mutexes to avoid race conditions between tasks.

## Signal Features

| Metric | Purpose |
|---|---|
| RMS | Overall vibration severity |
| Peak | Maximum vibration excursion |
| Crest factor | Highlights impulsive mechanical events |
| Dominant frequency | Strongest spectral component |
| Bearing indicator | Relative energy in the configured 100–180 Hz band |
| Health indicator | Normalized RMS-based condition indicator from 0–100% |

The health indicator is a **heuristic engineering indicator**, derived from measured RMS relative to a configured critical threshold. It should not be interpreted as a probabilistic failure prediction.

## Hardware

### Edge node

- ESP32-WROOM-32
- 2 × MPU6050 accelerometer/gyroscope modules
- TB6612FNG dual motor driver
- 2 × DC motors
- Passive decoupling / power filtering

### Local HMI

- ESP8266 / IdeaSpark board
- Integrated SSD1306 OLED
- PCF8574 I/O expander
- 4×4 matrix keypad
- Buzzer for local warning/critical alerts

## Hardware Design Decisions

### Dual-Bus I2C

Two hardware I2C buses are used to isolate the vibration sensors:

- **Bus 0:** SDA 21 / SCL 22 at 400 kHz
- **Bus 1:** SDA 18 / SCL 19 at 100 kHz

This avoids the need for an external I2C multiplexer while allowing two identical sensors to be used simultaneously.

During integration, one non-standard MPU6050-compatible sensor reported an atypical silicon identifier. The initialization path was adapted to support the hardware encountered during testing.

### Motor drive and EMC mitigation

The motors are driven by a TB6612FNG at **20 kHz PWM** with 8-bit duty-cycle control. The prototype also uses local decoupling and bulk capacitance to reduce power-rail disturbances caused by the inductive motor loads.

## Communications

The project uses multiple communication paths for different purposes:

- **UDP:** low-latency HMI discovery, telemetry, and operator commands
- **MQTT:** secondary telemetry path for a central monitoring station
- **WebSocket:** live dashboard updates
- **HTTP:** self-hosted SPA resources from ESP32 flash
- **JSON:** telemetry and command serialization

The HMI implements local network discovery and communicates asynchronously with the ESP32 edge node.

## Web Dashboard

The ESP32 serves a Single Page Application directly from SPIFFS. The dashboard provides live motor metrics, FFT visualization, state information, and operator controls without requiring an external cloud service.

Static assets are cached by the browser to reduce repeated traffic and load on the ESP32 network core.

## Experimental Validation

The prototype was tested using two mechanically coupled DC motor assemblies representing nominal and faulted operation.

Validation included:

- Baseline/noise-floor characterization
- Normal vs. mechanically faulted motor comparison
- Induced rotor imbalance
- Mechanical crosstalk between motors on a shared structure
- Spectral fault-signature extraction
- Health-indicator response
- Network latency measurements
- Browser resource-loading/caching measurements

A fault-related spectral component around **43.8 Hz** was identified during the documented imbalance experiment. The system also demonstrated sub-20 ms average reaction time for the low-latency safety/control path in the documented test setup.

See [`docs/VALIDATION.md`](docs/VALIDATION.md) for the experimental summary.

## Repository Structure

```text
.
├── Edge_Computing32/        # ESP32 edge-processing firmware
│   ├── data/                # Web dashboard (HTML/CSS/JS)
│   ├── src/                 # Sensors, DSP, motors, networking
│   ├── platformio.ini
│   └── partitions_no_ota.csv
│
├── HMI-8266/                # ESP8266 local HMI firmware
│   ├── src/                 # Display, input, networking, state
│   └── platformio.ini
│
└── docs/
    ├── ARCHITECTURE.md
    └── VALIDATION.md
```

## Toolchain and Libraries

- C++17
- PlatformIO
- Arduino framework
- FreeRTOS
- ArduinoFFT
- ArduinoJson
- Adafruit MPU6050
- AsyncTCP
- ESPAsyncWebServer
- PubSubClient (MQTT)
- U8g2
- SPIFFS

## Build

Both embedded targets are PlatformIO projects.

### ESP32 edge node

```bash
cd Edge_Computing32
pio run
```

### ESP8266 HMI

```bash
cd HMI-8266
pio run
```

Hardware-specific network credentials, pin assignments, and thresholds should be reviewed in the configuration headers before flashing.

## Current Project Scope

Implemented:

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

Planned / research directions:

- [ ] Long-term condition history and trend analysis
- [ ] Dataset collection across multiple fault classes
- [ ] TinyML anomaly detection on the edge
- [ ] NTP timestamp synchronization
- [ ] TLS for MQTT/WebSocket communications
- [ ] Automated threshold calibration
- [ ] Remaining Useful Life estimation

## Engineering Focus

This project explores how low-cost embedded hardware can move machine-condition analysis closer to the physical asset. Instead of continuously sending raw high-rate vibration data to a remote server, the edge node extracts compact diagnostic features locally and distributes only relevant telemetry.

The project combines **embedded systems, industrial communication, digital signal processing, motor control, IIoT, real-time software, and condition monitoring** in a single working prototype.

## Author

**Robert Constantin Preda**  
Master's programme: Information Technologies in Systems Engineering  
Project developed for the Embedded Systems Architecture course.
