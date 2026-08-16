#pragma once
#include <Arduino.h>
#include "secrets.h"

// -------- General configuration --------
inline constexpr int   SAMPLES  = 256;
inline constexpr float FS_HZ    = 400.0f;
inline constexpr int   FRAME_MS = int((SAMPLES / FS_HZ) * 1000.0f + 0.5f);

inline constexpr float RMS_WARN = 1.50f;
inline constexpr float RMS_CRIT = 3.00f;

// -------- I2C vibration sensors --------
inline constexpr int I2C_SDA  = 21;
inline constexpr int I2C_SCL  = 22;
inline constexpr int I2C2_SDA = 18;
inline constexpr int I2C2_SCL = 19;

// -------- Network --------
// WIFI_SSID, WIFI_PASS, AP_SSID and AP_PASS are defined in secrets.h.
inline constexpr const char* HOSTNAME = "esp32-motor-unit";

inline constexpr const char* MQTT_HOST   = "192.168.100.3";
inline constexpr uint16_t    MQTT_PORT   = 1883;
inline constexpr const char* MQTT_CLIENT = "esp32-pdm";
inline constexpr const char* MQTT_TOPIC_TELE = "pdm/telemetry";

inline constexpr uint16_t HTTP_PORT = 80;
inline constexpr uint16_t WS_PORT   = 81;
inline constexpr uint16_t UDP_PORT  = 3333;

// -------- TB6612FNG motor driver --------
inline constexpr int PIN_STBY = 27;

// Motor 1 / channel A
inline constexpr int PIN_PWMA = 33;
inline constexpr int PIN_AIN1 = 26;
inline constexpr int PIN_AIN2 = 25;

// Motor 2 / channel B
inline constexpr int PIN_PWMB = 32;
inline constexpr int PIN_BIN1 = 14;
inline constexpr int PIN_BIN2 = 13;
