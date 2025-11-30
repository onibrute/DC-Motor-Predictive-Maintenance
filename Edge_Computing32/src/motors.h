// motors.h
#pragma once
#include "config.h"

extern int motor_profile;     

void motors_init();
void motorA_set(int v);
void motorB_set(int v);
void motors_stop_all();
void motors_applyProfile(int p);
void driverStandby(bool en);
void estop_set(bool on);
bool estop_is_on();

int motorA_get_pwm();
int motorB_get_pwm();