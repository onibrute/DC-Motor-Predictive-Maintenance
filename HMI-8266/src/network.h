#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "types.h"

// ===== Inițializare Wi-Fi + UDP =====
void network_init();

// Loop periodic (menține conexiunea, reconectează dacă pică)
void network_loop();

// Trimite o comandă UDP către ESP32
void network_sendCommand(const String& cmd);

// Returnează true dacă Wi-Fi e activ
bool network_isConnected();

// IP-ul curent al serverului (ESP32)
IPAddress network_getServerIP();
