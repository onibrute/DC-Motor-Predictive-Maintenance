#pragma once
#include <Arduino.h>
#include "types.h"

// ===== Inițializare ecran =====
void display_init();

// Redesenare completă a ecranului curent (apelat periodic)
void display_update();

// Mesaj simplu centrat pentru un interval scurt
void display_showMessage(const char* msg, uint16_t duration_ms);

// Forțează trezirea ecranului din mod „dimmed”
void display_wake();

// Resetează timerul de activitate
void display_touchActivity();

// Controlează dacă ecranul e în power-save
bool display_isDimmed();
