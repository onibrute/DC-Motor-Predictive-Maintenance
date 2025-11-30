#pragma once
#include "config.h"
#include "utils.h"

// Initialize Wi-Fi, mDNS, UDP, WebServer, MQTT
bool network_init();

// Main network loop (handles incoming UDP commands & MQTT)
void network_loop();

// Publish data to HMI (via UDP) and Cloud (via MQTT/WS)
void network_publish(const FrameMetrics& fm1, const FrameMetrics& fm2,
                     const float* fft1, const float* fft2, int fft_len,
                     float rms1, float rms2);