# Communications Architecture

## Overview

The prototype uses different network paths for different roles instead of forcing all traffic through one protocol:

| Transport / protocol | Role |
|---|---|
| Custom UDP (“QUIC-lite”) | Local HMI discovery, telemetry and low-latency commands |
| MQTT over TCP | Secondary monitoring/CLI telemetry |
| WebSocket | Live browser telemetry and FFT updates |
| HTTP | Dashboard assets and configuration pages |

## “QUIC-lite” — what the name means

The project documentation uses **QUIC-lite** as a descriptive name for the custom UDP communication path between the ESP8266 HMI and ESP32 edge node.

The design is **inspired by QUIC’s low-latency, UDP-based philosophy**, especially the desire to avoid the setup overhead of a conventional TCP control connection. However, this project is **not an implementation of IETF QUIC (RFC 9000)**.

Real QUIC is a secure general-purpose transport carried over UDP and includes transport mechanisms such as connection establishment, cryptographic protection, packet-number spaces, connection IDs, multiplexed streams, loss recovery and congestion control. The project protocol does not reproduce that wire format or feature set.

## Implemented UDP behavior

### 1. Local discovery

When the HMI does not know the edge-node address, it scans the local /24 subnet and sends:

```text
PING_DISCOVERY
```

to candidate addresses on the configured UDP port. The HMI learns the ESP32 address from the source of an incoming datagram.

This makes the HMI usable without a hard-coded ESP32 IP address. The firmware also contains mDNS support on the ESP32, but the current HMI discovery implementation shown in the repository uses the UDP subnet sweep.

### 2. Telemetry

The edge node sends compact JSON telemetry containing motor state and diagnostic features. The HMI parser currently consumes fields such as:

- motor PWM / speed state;
- RMS vibration;
- dominant frequency;
- crest factor.

The browser receives a richer live stream separately through WebSocket, including FFT data.

### 3. Direct commands

The ESP32 UDP handler accepts simple local commands including forms equivalent to:

```text
A=<PWM>
B=<PWM>
STOP
ESTOP
```

This path avoids establishing a TCP session before sending an operator command.

## What is QUIC-inspired

The similarities are conceptual rather than protocol-compatible:

- UDP as the underlying datagram transport;
- minimizing setup latency for the local control path;
- application-controlled message semantics;
- rapid discovery/communication on the local network.

## What is NOT implemented

The project does not currently implement:

- IETF QUIC packet format;
- TLS 1.3 integrated into the transport handshake;
- encrypted QUIC payloads;
- QUIC streams or stream multiplexing;
- connection IDs;
- QUIC acknowledgement/loss-recovery logic;
- congestion control;
- path migration;
- standards-compliant QUIC 0-RTT resumption.

For that reason, documentation and portfolio material should use **“custom QUIC-inspired UDP protocol”** or **“QUIC-lite (project terminology)”**, not simply “QUIC implementation.”

## MQTT path

MQTT is retained as a separate secondary telemetry route toward a central monitoring station / CLI. This provides a conventional publish/subscribe integration point for future persistence, database ingestion or cloud gateways without placing that overhead on the local HMI command path.

## WebSocket path

The browser dashboard uses WebSocket for live telemetry and FFT data. This allows the UI to update without repeatedly polling HTTP endpoints.

## HTTP and dashboard assets

The ESP32 serves the SPA from SPIFFS and applies browser caching headers. The current HTML still references Chart.js from `cdn.jsdelivr.net`, so the firmware-hosted UI is not yet completely independent of internet access for chart rendering. For a fully offline industrial-style deployment, Chart.js should be stored in `Edge_Computing32/data/` and served locally.

## Security considerations

The current prototype prioritizes experimentation and local-network functionality. Before use beyond a controlled lab/LAN environment, recommended work includes:

- authentication for motor-control commands;
- message integrity / replay protection;
- TLS for MQTT and browser communications;
- secure provisioning of Wi-Fi credentials;
- rate limiting and stricter command validation;
- replacing subnet scanning where network policy requires a managed discovery mechanism.

## Reference

IETF QUIC is specified by RFC 9000: https://www.rfc-editor.org/rfc/rfc9000
