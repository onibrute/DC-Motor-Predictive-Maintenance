#include "motors.h"
#include <Arduino.h>
int motor_profile = 0;

static volatile bool g_estop = false;
static volatile bool g_stby  = true;

// LEDC (PWM)
static constexpr uint32_t LEDC_FREQ = 20000; // 20 kHz
static constexpr uint8_t  LEDC_RES  = 8;     // 8-bit
static constexpr uint8_t  LEDC_CH_A = 0;
static constexpr uint8_t  LEDC_CH_B = 1;

// NEW: reținem ultimul PWM aplicat (0..255, semn ignorat)
static volatile int g_pwm_a = 0;
static volatile int g_pwm_b = 0;

static inline uint8_t pwmClamp(int v) {
  if (v < 0) v = -v;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

void driverStandby(bool en) {
  g_stby = en;
  digitalWrite(PIN_STBY, en ? HIGH : LOW);
}

bool estop_is_on() { return g_estop; }

void estop_set(bool on) {
  g_estop = on;
  if (on) {
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
    ledcWrite(LEDC_CH_A, 0);     ledcWrite(LEDC_CH_B, 0);
    g_pwm_a = 0; g_pwm_b = 0; // NEW
  }
}

void motors_init() {
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);

  ledcSetup(LEDC_CH_A, LEDC_FREQ, LEDC_RES);
  ledcSetup(LEDC_CH_B, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(PIN_PWMA, LEDC_CH_A);
  ledcAttachPin(PIN_PWMB, LEDC_CH_B);

  digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
  ledcWrite(LEDC_CH_A, 0); ledcWrite(LEDC_CH_B, 0);
  g_pwm_a = 0; g_pwm_b = 0;

  driverStandby(true);
}

void motorA_set(int v) {
  if (g_estop || !g_stby) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    ledcWrite(LEDC_CH_A, 0);
    g_pwm_a = 0;
    return;
  }

  if (v == 0) {
    // Active brake: short both motor terminals
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    ledcWrite(LEDC_CH_A, 0);
    g_pwm_a = 0;
    return;
  }

  if (v > 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
  }

  uint8_t p = pwmClamp(v);
  ledcWrite(LEDC_CH_A, p);
  g_pwm_a = p;
}


void motorB_set(int v) {
  if (g_estop || !g_stby) {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    ledcWrite(LEDC_CH_B, 0);
    g_pwm_b = 0;
    return;
  }

  if (v == 0) {
    // Active brake
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    ledcWrite(LEDC_CH_B, 0);
    g_pwm_b = 0;
    return;
  }

  if (v > 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
  }

  uint8_t p = pwmClamp(v);
  ledcWrite(LEDC_CH_B, p);
  g_pwm_b = p;
}


void motors_stop_all() {
  motorA_set(0);
  motorB_set(0);
}

void motors_applyProfile(int p) {
  motor_profile = p;
  switch (motor_profile) {
    case 0: // Normal - ACUM CU SOFT START
      // Pornim ușor ca să nu sară BMS-ul bateriei
      for (int speed = 50; speed <= 180; speed += 10) {
          motorA_set(speed);
          motorB_set(speed);
          delay(30); // Așteptăm 30ms între pași
      }
      // Asigurăm valoarea finală
      motorA_set(180);
      motorB_set(180);
      break;

    case 1: // Economy (~60%)
      // Putem aplica și aici un mini soft-start dacă e nevoie
      motorA_set(120);
      motorB_set(120);
      break;

    case 2: // Sync: B după 3s
      motorA_set(180);
      delay(3000);
      motorB_set(180);
      break;
  }
}

int motorA_get_pwm() { return g_pwm_a; }
int motorB_get_pwm() { return g_pwm_b; }