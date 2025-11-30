#pragma once

#include "config.h"

// Init MPU6050 (2 senzori pe 0x69 si 0x68)
bool sensors_init();

// Umple bufferul pentru FFT & RMS pentru AMBELE motoare
// vReal1/vImag1 -> Motor 1 (MPU1)
// vReal2/vImag2 -> Motor 2 (MPU2)
// rms1_out, rms2_out -> RMS pe fiecare motor
void sensors_fillFrame(double vReal1[], double vImag1[],
                       double vReal2[], double vImag2[],
                       float& rms1_out, float& rms2_out);