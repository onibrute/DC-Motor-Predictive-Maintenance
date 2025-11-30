#pragma once
#include <Arduino.h>

enum class ScreenState : uint8_t {
  SCREEN_SPLASH = 0,
  SCREEN_MAIN_MENU,
  SCREEN_MOTOR_STATUS,
  SCREEN_MOTOR_CONTROL,
  SCREEN_SETTINGS,
  SCREEN_ABOUT
};

enum class AlertState : uint8_t {
  NONE = 0,
  WARNING,
  CRITICAL
};

struct MotorData {
  bool  state;
  float rms;
  float frequency;
  int   speed;
  float crestFactor;
};
