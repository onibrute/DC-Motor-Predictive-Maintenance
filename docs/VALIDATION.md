# Experimental Validation

## Purpose

The prototype was validated on a two-motor test bench to determine whether the embedded DSP pipeline could distinguish nominal operation from mechanically faulted operation and expose the result through the distributed monitoring interfaces.

The documented test setup used two mechanically coupled DC motor assemblies mounted on a common structure. This intentionally reproduces a practical challenge found in machinery installations: vibration from one machine can propagate into another through the shared mechanical support.

## Test Strategy

The validation was divided into two main operating regimes:

1. **Static baseline** — characterize the measurement-chain noise floor with the motors at rest.
2. **Dynamic stress test** — compare vibration spectra under normal and induced-fault conditions.

The project documentation describes both intentionally worn / misaligned behavior and a controlled rotor-imbalance experiment used to validate spectral anomaly detection.

## Baseline / Noise Floor

The baseline test was used to determine whether the sensing and processing chain produced significant false vibration content while the system was at rest.

The documented conclusion was that the system retained a sufficiently low noise floor to distinguish real mechanical vibration from the stationary baseline without producing false alarms in the tested setup.

## Fault Injection and Mechanical Crosstalk

For dynamic validation, a mechanical imbalance was induced using an eccentric mass attached to the rotor of one motor while both motors operated on the same mechanically coupled structure.

This test was important for two reasons:

- it created a repeatable vibration anomaly;
- it demonstrated that the system could observe both the primary fault source and vibration propagated mechanically into the second motor.

The documented spectral comparison shows the faulted motor producing the stronger vibration signature while the second motor also registers induced vibration because of the common mechanical structure.

## Spectral Detection

The embedded DSP pipeline detected a fault-related spectral component at approximately **43.8 Hz** during the documented imbalance experiment.

The processing chain combines:

- offset / mean removal
- Hamming windowing
- FFT calculation
- dominant-frequency extraction
- RMS measurement
- spectral-energy analysis

This allows the prototype to distinguish changes that are not obvious from a single raw acceleration sample.

## Health Indicator

The system exposes an operator-friendly health indicator derived from RMS vibration relative to a configured critical RMS threshold:

```text
Health (%) = 100 × (1 - RMS_measured / RMS_critical)
```

The result is clamped to the range 0–100%.

This metric is best interpreted as a **normalized engineering condition indicator**, not a probability of failure and not a Remaining Useful Life estimate.

## Bearing-Band Indicator

The firmware also calculates the fraction of spectral energy contained in the configured **100–180 Hz** band. In the project this is used as a bearing-oriented condition indicator intended to highlight changes concentrated in that portion of the vibration spectrum.

The current implementation is therefore a feature-engineering / condition-monitoring approach rather than a trained fault-classification model.

## Network and Interface Performance

The distributed communication architecture was also evaluated experimentally.

### Low-latency path

The documented UDP-based safety/control path achieved an average reaction time below **20 ms** in the prototype test environment.

This path is intentionally separated from the secondary MQTT telemetry channel, allowing fast operator/safety interactions while retaining a more conventional publish/subscribe route for monitoring data.

### Web dashboard caching

The project documentation reports browser-loading measurements for the ESP32-hosted dashboard:

| Scenario | Documented load time |
|---|---:|
| First / cold load | 645 ms |
| Cached reload | 152 ms |

Client-side HTTP caching reduces repeated static-asset transfers and lowers the load on the ESP32 networking core during subsequent dashboard access.

## What the Validation Supports

The experiments support the following claims for the tested prototype:

- real-time vibration acquisition from two motors;
- local FFT-based feature extraction on the ESP32;
- detection of a repeatable mechanical imbalance signature;
- observation of vibration propagation / crosstalk through a shared support;
- responsive local/distributed operator feedback;
- feasibility of edge-based condition monitoring without mandatory cloud processing.

## What It Does Not Yet Establish

The current experimental work does **not** establish:

- Remaining Useful Life (RUL) prediction;
- statistically validated failure probability;
- generalization across motor families and operating loads;
- compliance certification against an industrial predictive-maintenance standard;
- a trained ML model for automated multi-class fault diagnosis.

Those would require broader datasets, repeated controlled trials, calibrated instrumentation, and additional validation.

## Next Validation Steps

Recommended extensions include:

1. Collect long-duration vibration histories for healthy and progressively degraded motors.
2. Repeat measurements across multiple motor speeds and mechanical loads.
3. Label fault classes such as imbalance, misalignment, looseness, and bearing degradation.
4. Compare embedded features against reference instrumentation.
5. Calculate repeatability, false-positive rate, and detection sensitivity.
6. Train and evaluate a TinyML anomaly detector only after a sufficiently representative dataset exists.
7. Add precise NTP timestamps for trend correlation.
8. Add secure TLS transport before moving toward networked industrial deployment.
