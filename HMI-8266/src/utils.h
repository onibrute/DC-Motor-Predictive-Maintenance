#pragma once
#include <Arduino.h>

// ===== Buzzer =====
void buzzer_init();
void buzzer_beep(int freq, int dur);
void buzzer_update();

// ===== Funcții generale =====
float utils_clamp(float x, float a, float b);
