#include "settings.h"
#include "config.h"   
#include "state.h"    
#include "types.h"    
#include <EEPROM.h>


// ====== Implementare completă ======

void settings_load() {
  EEPROM.begin(512);
  EEPROM.get(EEPROM_ADDR_WARN, g_warningThreshold);
  EEPROM.get(EEPROM_ADDR_CRIT, g_criticalThreshold);
  EEPROM.end();

  if (isnan(g_warningThreshold) || g_warningThreshold < 0 || g_warningThreshold > 1000)
    g_warningThreshold = 10.0f;

  if (isnan(g_criticalThreshold) || g_criticalThreshold < 0 || g_criticalThreshold > 1000)
    g_criticalThreshold = 20.0f;

  if (g_criticalThreshold <= g_warningThreshold)
    g_criticalThreshold = g_warningThreshold + 5.0f;

  Serial.printf("[SETTINGS] Loaded from EEPROM: Warn=%.1f, Crit=%.1f\n",
                g_warningThreshold, g_criticalThreshold);
}

void settings_save() {
  EEPROM.begin(512);
  EEPROM.put(EEPROM_ADDR_WARN, g_warningThreshold);
  EEPROM.put(EEPROM_ADDR_CRIT, g_criticalThreshold);

  if (EEPROM.commit())
    Serial.println("[SETTINGS] Saved to EEPROM successfully");
  else
    Serial.println("[SETTINGS] EEPROM commit failed!");

  EEPROM.end();
}

// ====== Accessori ======
float settings_getWarn() { return g_warningThreshold; }
float settings_getCrit() { return g_criticalThreshold; }

void settings_setWarn(float val) { g_warningThreshold = val; }
void settings_setCrit(float val) { g_criticalThreshold = val; }
