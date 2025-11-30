#include "utils.h"
#include "config.h"   
#include "state.h"    
#include "types.h"    

// ====== Buzzer ======
void buzzer_init() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzer_beep(int freq, int dur) {
  tone(BUZZER_PIN, freq, dur);
}

void buzzer_update() {
  static unsigned long lastBeep = 0;
  static bool buzzing = false;

  if (g_alertState == AlertState::CRITICAL) {
    if (millis() - lastBeep > (buzzing ? 200 : 300)) {
      buzzing = !buzzing;
      if (buzzing) tone(BUZZER_PIN, 2000, 200);
      else noTone(BUZZER_PIN);
      lastBeep = millis();
    }
  } else {
    if (buzzing) {
      noTone(BUZZER_PIN);
      buzzing = false;
    }
  }
}

// ====== Clamp float ======
float utils_clamp(float x, float a, float b) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}
