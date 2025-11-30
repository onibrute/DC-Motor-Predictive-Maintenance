#include "input.h"
#include "config.h"
#include "state.h"
#include "display.h"
#include "network.h"
#include "settings.h"
#include "utils.h"
#include <Wire.h>
#include <PCF8574.h>

// =========================
// HARDWARE CONFIG
// =========================
#define I2C_SDA_PIN D6
#define I2C_SCL_PIN D5
#define PCF8574_ADDR 0x20 

PCF8574 pcf(PCF8574_ADDR);

static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// PIN MAPPING FIX (Nu mai ghicim)
// Rows (Iesiri) = P0, P1, P2, P3
// Cols (Intrari) = P4, P5, P6, P7
static const uint8_t rowPins[4] = {0, 1, 2, 3};
static const uint8_t colPins[4] = {4, 5, 6, 7};

// =========================
// Init
// =========================
void input_init() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!pcf.begin()) {
    Serial.println("[INPUT] PCF8574 NOT FOUND!");
  }
  // Force I2C fix
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // 1. Setăm totul pe HIGH inițial (Input Pullup)
  for(int i=0; i<8; i++) pcf.write(i, HIGH);
}

// =========================
// Citire Tastatură (FĂRĂ SCANARE INVERSĂ)
// =========================
static char readKeypad() {
  
  // Scanăm DOAR direcția corectă: P0-P3 sunt Linii
  for (int r = 0; r < 4; r++) {
    
    // Activăm linia curentă (LOW)
    pcf.write(rowPins[r], LOW); 
    
    // Verificăm coloanele
    for (int c = 0; c < 4; c++) {
      if (pcf.read(colPins[c]) == LOW) {
        char key = keymap[r][c];
        
        // Debounce mic
        delay(30); 
        
        // Așteptăm să iei degetul (cu Timeout de siguranță)
        unsigned long t = millis();
        while(pcf.read(colPins[c]) == LOW) {
            yield(); // Previne crash Watchdog
            if(millis() - t > 1000) break; // Dacă e blocat > 1 sec, ieșim forțat
        }
        
        // Dezactivăm linia la loc (HIGH)
        pcf.write(rowPins[r], HIGH);
        return key;
      }
    }
    // Dezactivăm linia la loc (HIGH) înainte să trecem la următoarea
    pcf.write(rowPins[r], HIGH);
  }
  
  return '\0';
}

// =========================
// Loop
// =========================
void input_loop() {
    char key = readKeypad();
    if (!key) return;

    Serial.print("[KEY] "); Serial.println(key);

    buzzer_beep(1500, 50);
    display_wake();
    display_touchActivity();

    switch (g_currentState) {
        case ScreenState::SCREEN_MAIN_MENU:
            if (key == '2') g_menuIndex = (g_menuIndex - 1 + g_maxMenuItems) % g_maxMenuItems;
            if (key == '8') g_menuIndex = (g_menuIndex + 1) % g_maxMenuItems;
            if (key == '5') {
                switch (g_menuIndex) {
                    case 0: g_motorToDisplay = 1; g_currentState = ScreenState::SCREEN_MOTOR_STATUS; break;
                    case 1: g_motorToDisplay = 2; g_currentState = ScreenState::SCREEN_MOTOR_STATUS; break;
                    case 2: 
                        g_motorControlIndex = 0; 
                        g_motor1SpeedOverride = 0; 
                        g_motor2SpeedOverride = 0;
                        g_currentState = ScreenState::SCREEN_MOTOR_CONTROL; 
                        break;
                    case 3: g_currentState = ScreenState::SCREEN_SETTINGS; g_settingsIndex = 0; break;
                    case 4: g_currentState = ScreenState::SCREEN_ABOUT; break;
                }
            }
            break;

        case ScreenState::SCREEN_MOTOR_STATUS:
            if (key == '*') g_currentState = ScreenState::SCREEN_MAIN_MENU;
            break;

        case ScreenState::SCREEN_MOTOR_CONTROL:
            // Control Manual
            if (key == '2') g_motorControlIndex = 0;
            if (key == '8') g_motorControlIndex = 1;
            
            {
                int* spd = (g_motorControlIndex == 0) ? &g_motor1SpeedOverride : &g_motor2SpeedOverride;
                if (key == '4') *spd = constrain(*spd - 50, 0, 255); // Limitat 0-255
                if (key == '6') *spd = constrain(*spd + 50, 0, 255);
            }

            if (key == '5') {
                // Trimite comanda
                int val = (g_motorControlIndex == 0) ? g_motor1SpeedOverride : g_motor2SpeedOverride;
                String cmd = ((g_motorControlIndex == 0) ? "A=" : "B=") + String(val);
                
                // Dacă avem rețea, trimitem. Dacă nu, afișăm eroare.
                network_sendCommand(cmd); 
                display_showMessage("Sent!", 1000);
            }
            
            if (key == '*') g_currentState = ScreenState::SCREEN_MAIN_MENU;
            break;

        case ScreenState::SCREEN_SETTINGS:
            if (key == '2') g_settingsIndex = 0;
            if (key == '8') g_settingsIndex = 1;
            {
                float* val = (g_settingsIndex == 0) ? &g_warningThreshold : &g_criticalThreshold;
                if (key == '4') *val = max(0.0f, *val - 0.5f);
                if (key == '6') *val += 0.5f;
            }
            if (key == '5') {
                settings_save();
                display_showMessage("Saved!", 1000);
                g_currentState = ScreenState::SCREEN_MAIN_MENU;
            }
            if (key == '*') {
                settings_load();
                g_currentState = ScreenState::SCREEN_MAIN_MENU;
            }
            break;

        case ScreenState::SCREEN_ABOUT:
            if (key == '*') { g_aboutPage = 0; g_currentState = ScreenState::SCREEN_MAIN_MENU; }
            if (key == '8') g_aboutPage = (g_aboutPage + 1) % 2;
            if (key == '2') g_aboutPage = (g_aboutPage == 0 ? 1 : 0);
            break;
            
        default: break;
    }
}