#pragma once
#include <Arduino.h>

// Inițializează și încarcă pragurile salvate
void settings_load();

// Salvează valorile curente în EEPROM
void settings_save();

// Accesori pentru pragurile de avertizare și critic
float settings_getWarn();
float settings_getCrit();

// Setează pragurile (nu salvează automat)
void settings_setWarn(float val);
void settings_setCrit(float val);
