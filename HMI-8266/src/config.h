#pragma once
#include <Arduino.h>

// ===== Wi-Fi & rețea (și la fel ca în codul tău mare) =====
#define WIFI_SSID         "DIGI-4Anw"
#define WIFI_PASS         "3WX57Ct9"
#define SERVER_HOSTNAME   "esp32-motor-monitor.local"   // mDNS al ESP32
#define UDP_PORT          3333

// ===== OLED (U8g2) – pini pentru ESP8266 (NodeMCU) =====
#define OLED_SCL          D5
#define OLED_SDA          D6

// ===== Buzzer =====
#define BUZZER_PIN        3   // GPIO3 (RX)

// ===== EEPROM layout =====
#define EEPROM_ADDR_WARN  0
#define EEPROM_ADDR_CRIT  4

// ===== Ecran / UI =====
#define SCREEN_DIM_TIMEOUT_MS  60000UL   // 1 minut

// ===== Keypad 4x4 (pini exact ca în proiectul tău) =====
#define KP_ROWS 4
#define KP_COLS 4
static const byte KP_ROW_PINS[KP_ROWS] = { D0, D1, D2, D4 }; // rânduri
static const byte KP_COL_PINS[KP_COLS] = { D5, D6, D7, D8 }; // coloane

// ===== PCF8574 Expander =====
#define I2C_SDA_PIN  D6  // OLED deja folosește SDA = D6 la tine
#define I2C_SCL_PIN  D5   
#define PCF8574_ADDR 0x20 
