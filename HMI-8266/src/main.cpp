#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "state.h"
#include "network.h"
#include "display.h"
#include "input.h"
#include "settings.h"
#include "utils.h"
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("===================");
  Serial.println("PdM HMI v1.0");
  Serial.println("===================");

  // Shared I2C bus on D6 (SDA), D5 (SCL)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  buzzer_init();
  display_init();    // uses u8g2 HW I2C (Wire)
  settings_load();
  input_init();      // NO Wire.begin() inside
  network_init();

  g_currentState   = ScreenState::SCREEN_MAIN_MENU;
  g_lastActivityMs = millis();

  Serial.println("[MAIN] Setup complete, entering loop");
  Serial.println("===================");
}


void loop() {
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    Serial.println("[MAIN] loop alive");
    last = millis();
  }

  network_loop();
  input_loop();
  buzzer_update();
  display_update();
  delay(20);
}

