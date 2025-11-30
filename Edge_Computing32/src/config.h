#pragma once
#include <Arduino.h>

// -------- Configurații Generale --------
inline constexpr int   SAMPLES = 256;        
inline constexpr float FS_HZ   = 400.0f;     
inline constexpr int   FRAME_MS = int((SAMPLES / FS_HZ) * 1000.0f + 0.5f);

inline constexpr float RMS_WARN = 80.0f;
inline constexpr float RMS_CRIT = 150.0f;

// -------- I2C (Senzori) --------
inline constexpr int I2C_SDA  = 21;
inline constexpr int I2C_SCL  = 22;

// Bus secundar pentru al doilea MPU
inline constexpr int I2C2_SDA = 18;
inline constexpr int I2C2_SCL = 19;


// -------- Wi-Fi & Rețea --------
inline constexpr const char* WIFI_SSID = "DIGI-4Anw";
inline constexpr const char* WIFI_PASS = "3WX57Ct9";
inline constexpr const char* AP_SSID   = "BOT_AP";
inline constexpr const char* AP_PASS   = "12345678";

inline constexpr const char* HOSTNAME  = "esp32-motor-unit"; 

// Web & MQTT
inline constexpr const char* MQTT_HOST = "192.168.100.3";
inline constexpr uint16_t    MQTT_PORT = 1883;
inline constexpr const char* MQTT_CLIENT = "esp32-pdm";
inline constexpr const char* MQTT_TOPIC_TELE = "pdm/telemetry";

inline constexpr uint16_t HTTP_PORT = 80;
inline constexpr uint16_t WS_PORT   = 81;

// UDP (HMI) - ATENȚIE: Să fie la fel ca pe ESP8266!
inline constexpr uint16_t UDP_PORT  = 3333; 

// -------- Driver Motoare (TB6612FNG) --------
// Pin STBY (Trebuie să fie HIGH)
inline constexpr int PIN_STBY = 27; 

// Motor 1 (Canal A)
inline constexpr int PIN_PWMA = 33;
inline constexpr int PIN_AIN1 = 26;
inline constexpr int PIN_AIN2 = 25;

// Motor 2 (Canal B)
inline constexpr int PIN_PWMB = 32;
inline constexpr int PIN_BIN1 = 14;
inline constexpr int PIN_BIN2 = 13;