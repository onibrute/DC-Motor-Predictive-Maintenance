# Experimental Validation

## Purpose

The prototype was validated on a mechanically coupled two-motor bench to determine whether the embedded DSP pipeline could distinguish stationary/nominal behavior from an induced mechanical anomaly and expose that distinction through the distributed monitoring interfaces.

The figures and values below are **results reported for this specific experimental setup**. They are not general motor specifications or certified industrial limits. The README now presents the original figures from the submitted project documentation rather than a synthetic validation summary.

## Test strategy

The report describes two principal regimes:

1. **Static baseline** — motors stopped, used to characterize the measurement-chain noise floor.
2. **Dynamic stress test** — motors running on a common mechanical support, with a controlled imbalance introduced on one motor to test spectral discrimination and mechanical crosstalk.

The report also discusses intentionally worn/misaligned behavior as part of the broader prototype context. The most quantitative validation presented is the controlled imbalance experiment.

## Baseline / noise floor

With the motors stopped, the report records stable RMS values around **0.03–0.05 g**. The spectrum was described as lacking large dominant harmonics, providing a practical baseline for the later dynamic test.

This establishes the measured stationary floor of the prototype as assembled; it should not be interpreted as the intrinsic noise specification of the MPU6050 itself.

## Induced imbalance experiment

A mass eccentricity was introduced on the rotor of the anomaly-source motor while both motors were mounted on the same support.

<p align="center"><img src="assets/fault-spectrum.jpg" alt="Fault spectrum from the documented imbalance experiment" width="82%"></p>

The report identifies a dominant component at approximately **43.8 Hz**. Interpreting that as a 1× rotational component gives:

```text
43.8 Hz × 60 ≈ 2628 RPM
```

The reported RMS values were approximately:

| Measurement point | Reported RMS |
|---|---:|
| Primary anomaly source | 2.02 g |
| Mechanically coupled neighboring motor | 0.14 g |

That corresponds to roughly **14×** separation in RMS magnitude for this test. The experiment therefore demonstrates that the common support transmitted vibration to the neighboring motor while the dominant source still remained distinguishable.

## Health indicator response

The firmware health indicator is calculated from RMS using the configured critical threshold:

```text
Health (%) = clamp(100 × (1 - RMS_measured / RMS_critical), 0, 100)
```

In the documented stress test, the report shows approximately:

- **33%** for the anomalous motor;
- **95%** for the nominal reference motor.

This value is a **normalized engineering visualization metric**. It is not a probability of failure, degradation model or Remaining Useful Life estimate.

## Bearing-oriented spectral feature

The firmware calculates the fraction of total FFT energy located in the configured **100–180 Hz** band. The project uses this as a bearing-oriented condition feature.

This band is a project-specific feature-engineering choice. Bearing characteristic frequencies depend on bearing geometry, shaft speed, mounting and measurement conditions; therefore this fixed band should not be treated as a universal bearing-fault rule.

## Network / interface validation

### Local UDP path

The report states an average local-network reaction time below **20 ms** for the tested low-latency UDP control/safety path.

The project calls this protocol **“QUIC-lite.”** The implementation is a custom UDP protocol inspired by QUIC's low-latency design goals, not an IETF QUIC implementation. See [`COMMUNICATIONS.md`](COMMUNICATIONS.md).

The reported latency is specific to the test LAN and measurement method; it is not a deterministic worst-case real-time guarantee.

### Multi-endpoint telemetry

The report documents simultaneous presentation of condition data on the browser dashboard and local HMI. In the implementation, the ESP32 publishes through separate paths: UDP toward the HMI and WebSocket toward browser clients, with MQTT retained as a secondary telemetry channel.

### Dashboard caching

The report records:

| Scenario | Reported load time |
|---|---:|
| Cold / first load | 645 ms |
| Cached reload | 152 ms |

This is consistent with the firmware adding HTTP `Cache-Control` headers for static resources.

However, the current repository's `index.html` imports Chart.js from `cdn.jsdelivr.net`. Therefore the current dashboard is **locally hosted but not yet fully offline/self-contained**. The report's stronger statement of complete external independence does not exactly match the checked-in implementation.

## Thresholds and standards caveat

The current firmware uses:

```text
RMS_WARN = 1.50 g
RMS_CRIT = 3.00 g
```

The report associates these values with ISO 10816 / a small-machine class. The repository does not contain enough evidence to establish that these exact **acceleration-in-g** thresholds are directly prescribed by the cited standard for this measurement setup.

For portfolio and technical documentation purposes, they are therefore described as **project-configured experimental thresholds**.

## What the validation supports

For the documented prototype, the experiments support claims of:

- two-channel vibration acquisition;
- local FFT-based feature extraction on an ESP32;
- detection of a repeatable induced imbalance signature;
- observation and discrimination of mechanically propagated vibration;
- responsive multi-interface telemetry;
- feasibility of edge-based condition monitoring without continuously streaming raw samples to a remote compute system.

## What it does not yet establish

The current work does **not** establish:

- Remaining Useful Life prediction;
- statistically validated failure probability;
- a generalizable bearing-fault detector;
- generalization across motor families, loads and speeds;
- deterministic worst-case networking latency;
- industrial certification or standards compliance;
- a trained multi-class fault-classification model.

## Recommended next validation stage

1. Use calibrated reference vibration instrumentation alongside the MPU6050 channels.
2. Repeat each condition over multiple runs and report uncertainty / repeatability.
3. Collect multiple motors and multiple operating speeds and loads.
4. Build controlled classes such as imbalance, misalignment, looseness and bearing degradation.
5. Separate training, validation and test datasets before introducing TinyML.
6. Measure false-positive rate, false-negative rate, sensitivity and specificity.
7. Record long-duration degradation histories before attempting RUL estimation.
8. Add precise time synchronization for event/trend correlation.
9. Select the applicable current vibration standard and measure the required physical quantity accordingly.
10. Harden/authenticate the network control path before deployment outside a controlled LAN.

For the full source-vs-firmware review, see [`DOCUMENTATION_AUDIT.md`](DOCUMENTATION_AUDIT.md).
