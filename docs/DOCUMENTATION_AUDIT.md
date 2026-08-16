# Documentation Audit

This file records a cross-check between the submitted 42-page technical report and the firmware currently stored in the repository. The purpose is to preserve the useful engineering content while clearly marking report statements that are outdated, stronger than the code supports, or internally inconsistent.

## Confirmed by both report and firmware

The following points are directly supported by the repository implementation and the technical report:

- ESP32-WROOM-32 as the main edge node;
- ESP8266-based local HMI;
- two MPU6050-compatible vibration sensors;
- 400 Hz configured sampling frequency;
- 256-sample processing frames;
- dual hardware I2C buses on the ESP32;
- FreeRTOS task pinning, with sensing/DSP on Core 1 and networking on Core 0;
- mutex-protected sharing of metrics / FFT buffers;
- Hamming windowing and FFT-based feature extraction;
- RMS, peak, crest factor, dominant-frequency and spectral-band metrics;
- TB6612FNG motor control using 20 kHz PWM;
- WebSocket browser telemetry;
- MQTT secondary telemetry;
- custom UDP communication between the ESP8266 HMI and ESP32;
- SPIFFS-based hosting of the dashboard files;
- client-side HTTP cache headers;
- PCF8574-based keypad expansion on the HMI;
- project-specific support for an MPU6050-compatible device reporting `WHO_AM_I = 0x72`.

## Correction 1 — “QUIC-lite” is QUIC-inspired UDP, not QUIC

The report explicitly describes the protocol as a **custom UDP protocol inspired by QUIC**. The firmware confirms a lightweight application protocol built on `WiFiUDP`, including subnet scanning, discovery probes, JSON telemetry and simple motor commands.

The implementation does **not** contain the transport mechanisms required for an IETF QUIC implementation. Therefore portfolio wording should be:

> **custom QUIC-inspired UDP protocol (“QUIC-lite” project terminology)**

and not:

> QUIC implementation

The report also uses the phrase “0-RTT handshake.” In standards-compliant QUIC, 0-RTT has a specific cryptographic/session-resumption meaning. The project does not implement that mechanism. Here the phrase should be interpreted only as a design analogy for rapid local communication without TCP connection setup.

## Correction 2 — discovery response behavior

The report describes a `PING_DISCOVERY` / `PONG_DISCOVERY` exchange. In the current repository code:

- the ESP8266 sends `PING_DISCOVERY` while scanning the local subnet;
- the ESP32 learns the HMI address from the received UDP packet;
- the HMI learns the ESP32 address from an incoming datagram;
- the current ESP32 handler does not contain an explicit `PONG_DISCOVERY` response string.

The repository documentation therefore describes discovery according to the code rather than claiming an explicit PONG message that is not presently implemented.

## Correction 3 — SPIFFS vs LittleFS

One early report section refers to **LittleFS**, while the implemented firmware, PlatformIO partitioning and later report sections use **SPIFFS**.

The current repository implementation is SPIFFS-based, so repository documentation uses **SPIFFS** consistently.

## Correction 4 — dashboard is not yet fully offline

The report describes the dashboard as independent of external resources and says libraries such as Chart.js are stored locally in flash. The current `Edge_Computing32/data/index.html` instead imports Chart.js from:

```text
https://cdn.jsdelivr.net/npm/chart.js
```

Therefore the current implementation is:

- self-hosted for the project's HTML/CSS/JavaScript files;
- not fully offline for Chart.js unless that resource is already cached by the browser.

A future cleanup should bundle Chart.js in the SPIFFS data directory and change the HTML to a local path.

## Correction 5 — ISO 10816 attribution

The report associates the configured acceleration thresholds (`RMS_WARN = 1.50 g`, `RMS_CRIT = 3.00 g`) with ISO 10816 / “Class I.” The firmware does contain those numerical thresholds, but the repository does **not** contain evidence demonstrating that those exact acceleration thresholds are directly prescribed by ISO 10816 for this test configuration.

Additionally, **ISO 10816-1:1995 is withdrawn and was replaced by ISO 20816-1:2016**.

For that reason, repository documentation treats 1.50 g and 3.00 g as **project-configured experimental thresholds**, not as certified ISO limits. Any future standards claim should identify the exact applicable ISO 20816 part, measurement quantity, machine class, operating condition and evaluation table.

## Correction 6 — health score interpretation

The health score is implemented as a linear inverse mapping from RMS to the configured critical threshold:

```text
Health = clamp(100 × (1 − RMS / RMS_CRIT), 0, 100)
```

This is an engineering visualization metric. It is not:

- a probability of failure;
- a calibrated degradation model;
- Remaining Useful Life;
- a machine-learning prediction.

## Validation values preserved from the report

The following are report-derived prototype observations and are retained with explicit experimental context:

- baseline RMS around 0.03–0.05 g;
- induced imbalance dominant component at 43.8 Hz;
- equivalent rotational speed around 2628 RPM;
- anomaly-source RMS around 2.02 g;
- neighboring mechanically coupled motor RMS around 0.14 g;
- approximately 14× magnitude separation in that experiment;
- reported health indicators around 33% and 95%;
- reported local-network reaction time below 20 ms;
- dashboard loading measurements of 645 ms cold / 152 ms cached.

These values should be presented as **results from the documented test setup**, not universal specifications.

## Areas requiring stronger future validation

For a more rigorous predictive-maintenance study, the next validation stage should include:

- calibrated reference instrumentation;
- repeated trials and confidence intervals;
- multiple motors of the same type;
- multiple speeds and loads;
- controlled fault classes;
- false-positive / false-negative statistics;
- long-duration degradation histories;
- standards-appropriate vibration quantities and measurement locations;
- comparison against an applicable current ISO 20816-series standard;
- a clearly separated training/test protocol before introducing TinyML.
