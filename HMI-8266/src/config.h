#pragma once
#include <Arduino.h>
#include "secrets.h"

// ===== Network =====
// WIFI_SSID and WIFI_PASS are defined in secrets.h.
#define SERVER_HOSTNAME   "esp32-motor-monitor.local"
#define UDP_PORT          3333

// ===== OLED (U8g2) =====
#define OLED_SCL          D5
#define OLED_SDA          D6

// ===== Buzzer =====
#define BUZZER_PIN        3

// ===== EEPROM layout =====
#define EEPROM_ADDR_WARN  0
#define EEPROM_ADDR_CRIT  4

// ===== Display / UI =====
#define SCREEN_DIM_TIMEOUT_MS  60000UL

// ===== 4x4 keypad =====
#define KP_ROWS 4
#define KP_COLS 4
static const byte KP_ROW_PINS[KP_ROWS] = { D0, D1, D2, D4 };
static const byte KP_COL_PINS[KP_COLS] = { D5, D6, D7, D8 };

// ===== PCF8574 I/O expander =====
#define I2C_SDA_PIN  D6
#define I2C_SCL_PIN  D5
#define PCF8574_ADDR 0x20
