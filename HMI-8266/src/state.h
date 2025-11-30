#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "types.h"

extern int g_aboutPage;

// ===== Conectivitate =====
extern bool      g_isWifiConnected;
extern IPAddress g_serverIp;

// ===== UI & Menuri =====
extern ScreenState g_currentState;
extern int  g_menuIndex;
extern const int g_maxMenuItems;
extern const char* g_menuItems[5];

extern int  g_motorToDisplay;
extern int  g_motorControlIndex;
extern int  g_motor1SpeedOverride;
extern int  g_motor2SpeedOverride;

extern int   g_settingsIndex;
extern float g_warningThreshold;
extern float g_criticalThreshold;

extern AlertState g_alertState;

// ===== Power Save / Refresh =====
extern unsigned long g_lastActivityMs;
extern bool          g_isDisplayDimmed;

// ===== Date motoare =====
extern MotorData g_motor1;
extern MotorData g_motor2;

// ===== Helper =====
void state_touchActivity();
