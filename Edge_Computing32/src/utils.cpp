#include "utils.h"
#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <arduinoFFT.h>

SystemConfig config;  // instanță globală

float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

// =====================================================
// Config management (save/load)
// =====================================================
void loadConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[Config] SPIFFS mount failed");
    return;
  }

  if (!SPIFFS.exists("/config.json")) {
    Serial.println("[Config] No config.json found, using defaults");
    config.wifi_ssid = WIFI_SSID;
    config.wifi_pass = WIFI_PASS;
    config.ap_ssid   = AP_SSID;
    config.ap_pass   = AP_PASS;
    saveConfig(config);
    return;
  }

  File f = SPIFFS.open("/config.json", "r");
  if (!f) {
    Serial.println("[Config] Failed to open config.json");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[Config] Parse error: %s\n", err.c_str());
    return;
  }

  config.wifi_ssid = doc["wifi_ssid"] | WIFI_SSID;
  config.wifi_pass = doc["wifi_pass"] | WIFI_PASS;
  config.ap_ssid   = doc["ap_ssid"]   | AP_SSID;
  config.ap_pass   = doc["ap_pass"]   | AP_PASS;
  Serial.println("[Config] Loaded from SPIFFS");
}

void saveConfig(const SystemConfig &cfg) {
  if (!SPIFFS.begin(true)) {
    Serial.println("[Config] SPIFFS mount failed");
    return;
  }

  File f = SPIFFS.open("/config.json", "w");
  if (!f) {
    Serial.println("[Config] Failed to open config.json for writing");
    return;
  }

  JsonDocument doc;
  doc["wifi_ssid"] = cfg.wifi_ssid;
  doc["wifi_pass"] = cfg.wifi_pass;
  doc["ap_ssid"]   = cfg.ap_ssid;
  doc["ap_pass"]   = cfg.ap_pass;
  serializeJson(doc, f);
  f.close();
  Serial.println("[Config] Saved to SPIFFS");
}

// =====================================================
// Semnal / FFT logic (codul tău original din utils.cpp)
// =====================================================
void signal_computeMetrics(double vReal[], double vImag[], FrameMetrics& out, float fs_hz, int samples) {
  double mean = 0.0;
  for (int i=0; i<samples; ++i) mean += vReal[i];
  mean /= (double)samples;
  for (int i=0; i<samples; ++i) vReal[i] -= mean;

  float peak_value = 0.0f;
  for (int i=0; i<samples; ++i) {
    float a = fabsf((float)vReal[i]);
    if (a > peak_value) peak_value = a;
  }

  double sum2 = 0.0;
  for (int i=0; i<samples; ++i) sum2 += vReal[i]*vReal[i];
  float rms_time = sqrtf((float)(sum2 / (double)samples));
  float crest    = (rms_time > 0.f) ? (peak_value / rms_time) : 0.f;

  for (int i=0; i<samples; ++i) {
    double w = 0.54 - 0.46 * cos((2.0 * M_PI * i) / (samples - 1));
    vReal[i] *= w;
    vImag[i]  = 0.0;
  }

  ArduinoFFT<double> FFT(vReal, vImag, samples, fs_hz);
  FFT.windowing(vReal, samples, FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
  FFT.compute(vReal, vImag, samples, FFT_FORWARD);
  FFT.complexToMagnitude();

  const float hzPerBin = fs_hz / (float)samples;
  int dcCut = (int)ceilf(10.0f / hzPerBin);
  if (dcCut > samples/2 - 1) dcCut = samples/2 - 1;
  for (int k=0; k<=dcCut; ++k) vReal[k] = 0.0;

  int maxBin = 1;
  float maxAmp = (float)vReal[1];
  for (int k=2; k<samples/2; ++k) {
    if (vReal[k] > maxAmp) { maxAmp = (float)vReal[k]; maxBin = k; }
  }
  float dom_freq = maxBin * hzPerBin;

  auto binFromHz = [&](float f)->int {
    int b = (int)lroundf(f / hzPerBin);
    if (b < 1) b = 1;
    if (b > (samples/2 - 1)) b = samples/2 - 1;
    return b;
  };
  int b_lo = binFromHz(100.0f);
  int b_hi = binFromHz(180.0f);

  double band_energy = 0.0, total_energy = 0.0;
  for (int k=1; k<samples/2; ++k) {
    double p = vReal[k]*vReal[k];
    total_energy += p;
    if (k >= b_lo && k <= b_hi) band_energy += p;
  }
  float bearing_index = (total_energy > 0.0) ? (float)(band_energy/total_energy) : 0.0f;

  float health = 100.0f * (1.0f - (rms_time / RMS_CRIT));
  health = clampf(health, 0.0f, 100.0f);

  out.rms           = rms_time;
  out.peak          = peak_value;
  out.crest         = crest;
  out.dom_freq_hz   = dom_freq;
  out.bearing_index = bearing_index;
  out.health        = health;
}
