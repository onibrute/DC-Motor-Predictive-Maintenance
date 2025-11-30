#pragma once
#include "config.h"

// Metrici pe frame (telemetrie)
struct FrameMetrics {
  float rms;            // [g]
  float peak;           // [g]
  float crest;          // peak / rms
  float dom_freq_hz;    // [Hz]
  float bearing_index;  // 0..1 (energie relativă 100–180 Hz)
  float health;         // 0..100
};

float clampf(float x, float a, float b);

// Calculează metrici + rescrie vReal[] cu |FFT| (N/2 utile în head)
// Pipeline: mean remove -> peak, RMS(t) -> Hamming -> FFT -> dom_freq -> bearing_index -> health
void signal_computeMetrics(double vReal[], double vImag[], FrameMetrics& out,
                           float fs_hz, int samples);

struct SystemConfig {
  String wifi_ssid;
  String wifi_pass;
  String ap_ssid;
  String ap_pass;
  int motor_speed;
  float rms_warning;
  float rms_critical;
};

extern SystemConfig config;
void loadConfig();
void saveConfig(const SystemConfig& cfg);

