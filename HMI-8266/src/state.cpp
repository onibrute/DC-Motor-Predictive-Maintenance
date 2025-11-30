#include "state.h"

int g_aboutPage = 0;
bool g_isWifiConnected = false;
IPAddress g_serverIp(0,0,0,0);

ScreenState g_currentState = ScreenState::SCREEN_MAIN_MENU;

int  g_menuIndex = 0;
const int g_maxMenuItems = 5;
const char* g_menuItems[5] = {
  "Motor 1 Status",
  "Motor 2 Status",
  "Manual Control",
  "Settings",
  "About"
};

// === CRITICAL VARIABLES ===
int  g_motorToDisplay      = 1;
int  g_motorControlIndex   = 0;
int  g_motor1SpeedOverride = 0; // Must be 0
int  g_motor2SpeedOverride = 0; // Must be 0

int   g_settingsIndex      = 0;
float g_warningThreshold   = 10.0f;
float g_criticalThreshold  = 20.0f;

AlertState g_alertState = AlertState::NONE;

unsigned long g_lastActivityMs = 0;
bool          g_isDisplayDimmed = false;

MotorData g_motor1 = { false, 0.0f, 0.0f, 0, 0.0f };
MotorData g_motor2 = { false, 0.0f, 0.0f, 0, 0.0f };

void state_touchActivity() {
  g_lastActivityMs = millis();
}