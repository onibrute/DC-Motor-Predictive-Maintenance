#include "display.h"
#include "config.h"
#include "state.h"
#include "types.h"
#include "utils.h"
#include <U8g2lib.h>
#include <Wire.h>

// ===== HARDWARE DEFINITIONS =====
#define I2C_SDA_PIN D6
#define I2C_SCL_PIN D5

// OLED Hardware Definition
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== Prototypes =====
static void drawWifiIndicator();
static void drawMainMenu();
static void displayMotorStatus(int motorNum);
static void displayMotorControl();
static void displaySettings();
static void displayAbout();

// ==========================================
// BOOT ANIMATION (No Overlap Version)
// ==========================================
static void display_bootAnimation() {
  // 1. Static Yellow Header
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(30, 14, "SYSTEM"); // Y=14 is safe in Yellow zone
  u8g2.sendBuffer();
  delay(200);

  // 2. Terminal Effect in Blue Zone
  const char* steps[] = { "CPU Init...", "Mem Check...", "Drivers...", "OK" };
  int yStart = 30; // Start deep in Blue zone
  
  u8g2.setFont(u8g2_font_6x10_tr);
  for(int i=0; i<4; i++) {
    u8g2.drawStr(10, yStart + (i*10), steps[i]);
    u8g2.sendBuffer();
    delay(150);
  }
  
  delay(500);
}

// ====== Initialization ======
void display_init() {
  // Force I2C pins (Safety)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!u8g2.begin()) {
    Serial.println("[DISPLAY] Failed!");
    // We continue anyway to avoid hanging logic
  }

  // Run Boot Animation
  display_bootAnimation();

  g_currentState = ScreenState::SCREEN_MAIN_MENU;
  g_lastActivityMs = millis();
  g_isDisplayDimmed = false;
}

// ===== Main Display Loop =====
void display_update() {
  if (g_isDisplayDimmed) return;

  u8g2.clearBuffer();
  
  // -- Draw based on State --
  switch (g_currentState) {
    case ScreenState::SCREEN_SPLASH:        break;
    case ScreenState::SCREEN_MAIN_MENU:     drawMainMenu(); break;
    case ScreenState::SCREEN_MOTOR_STATUS:  displayMotorStatus(g_motorToDisplay); break;
    case ScreenState::SCREEN_MOTOR_CONTROL: displayMotorControl(); break;
    case ScreenState::SCREEN_SETTINGS:      displaySettings(); break;
    case ScreenState::SCREEN_ABOUT:         displayAbout(); break;
  }
  
  drawWifiIndicator();
  u8g2.sendBuffer();

  // Auto Dimming Logic
  if (!g_isDisplayDimmed && (millis() - g_lastActivityMs > SCREEN_DIM_TIMEOUT_MS)) {
    g_isDisplayDimmed = true;
    u8g2.setPowerSave(1);
  }
}

void display_showMessage(const char* msg, uint16_t duration_ms) {
  u8g2.clearBuffer();
  // Draw a "Toast" box in the center
  u8g2.setFont(u8g2_font_ncenB08_tr);
  int w = u8g2.getStrWidth(msg) + 8;
  int x = (128 - w) / 2;
  
  u8g2.setDrawColor(0);
  u8g2.drawBox(x, 25, w, 15); // Erase background
  u8g2.setDrawColor(1);
  u8g2.drawFrame(x, 25, w, 15); // Border
  u8g2.drawStr(x+4, 36, msg);
  
  u8g2.sendBuffer();
  delay(duration_ms);
}

void display_wake() {
  if (g_isDisplayDimmed) {
    g_isDisplayDimmed = false;
    u8g2.setPowerSave(0);
  }
  g_lastActivityMs = millis();
}

void display_touchActivity() { g_lastActivityMs = millis(); }
bool display_isDimmed() { return g_isDisplayDimmed; }

static void drawWifiIndicator() {
  // Draw in Top-Right Yellow Zone
  u8g2.setFont(u8g2_font_5x8_tr); // Tiny font
  if (g_isWifiConnected) {
      u8g2.drawStr(110, 8, "WIFI");
  } else {
      u8g2.drawStr(110, 8, "--");
  }
}

// ==========================================
//  SCROLLING MENU LOGIC (The Downscroll Fix)
// ==========================================
static void drawMainMenu() {
  // 1. HEADER (Yellow Zone 0-15px)
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(2, 14, "MAIN MENU");
  u8g2.drawHLine(0, 16, 128); // Divider between Yellow/Blue

  // 2. CALCULATE SCROLL (Blue Zone 16-64px)
  // We have ~48px height. Each item is ~12px.
  // We can fit 3 items comfortably with spacing.
  const int ITEMS_PER_PAGE = 3;
  static int scrollOffset = 0;

  // Smart Scrolling: Move window if selection is out of bounds
  if (g_menuIndex >= scrollOffset + ITEMS_PER_PAGE) {
      scrollOffset = g_menuIndex - ITEMS_PER_PAGE + 1;
  }
  if (g_menuIndex < scrollOffset) {
      scrollOffset = g_menuIndex;
  }

  // 3. DRAW LIST
  u8g2.setFont(u8g2_font_ncenB08_tr);
  int yBase = 30; // Start drawing at Y=30 (well inside Blue zone)

  for (int i = 0; i < ITEMS_PER_PAGE; i++) {
    int itemIndex = scrollOffset + i;
    
    // Stop if we run out of items
    if (itemIndex >= g_maxMenuItems) break;

    int yPos = yBase + (i * 13); // 13px spacing

    if (itemIndex == g_menuIndex) {
      // Selected Item Style:
      // Draw a full-width box (Black text on White bg)
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, yPos - 9, 120, 11); 
      
      u8g2.setDrawColor(0); // Black Text
      u8g2.drawStr(5, yPos, g_menuItems[itemIndex]);
      
      // Scroll Bar Indicator
      u8g2.setDrawColor(1);
      u8g2.drawStr(115, yPos, "<");
    } else {
      // Normal Item Style
      u8g2.setDrawColor(1); // White Text
      u8g2.drawStr(5, yPos, g_menuItems[itemIndex]);
    }
  }
  
  // Restore draw color for safety
  u8g2.setDrawColor(1);
}

static void displayMotorStatus(int motorNum) {
  const MotorData& m = (motorNum == 1) ? g_motor1 : g_motor2;
  char buf[32];

  // HEADER (Yellow)
  u8g2.setFont(u8g2_font_helvB10_tr);
  sprintf(buf, "MOTOR %d", motorNum);
  u8g2.drawStr(2, 14, buf);
  u8g2.drawHLine(0, 16, 128);

  // BODY (Blue)
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  sprintf(buf, "State: %s", m.state ? "ON" : "OFF");
  u8g2.drawStr(5, 30, buf);
  
  sprintf(buf, "RPM  : %d", m.speed);
  u8g2.drawStr(5, 42, buf);

  sprintf(buf, "RMS  : %.2f", m.rms);
  u8g2.drawStr(5, 54, buf);

  // Footer
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(80, 63, "[*] BACK");
}

static void displayMotorControl() {
  char buf[32];
  
  // HEADER
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(2, 14, "CONTROL");
  u8g2.drawHLine(0, 16, 128);

  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Line 1: Motor 1
  sprintf(buf, "M1: %d", g_motor1SpeedOverride);
  if (g_motorControlIndex == 0) {
     u8g2.drawBox(0, 22, 128, 12);
     u8g2.setDrawColor(0);
  }
  u8g2.drawStr(5, 31, buf);
  u8g2.setDrawColor(1);

  // Line 2: Motor 2
  sprintf(buf, "M2: %d", g_motor2SpeedOverride);
  if (g_motorControlIndex == 1) {
     u8g2.drawBox(0, 35, 128, 12);
     u8g2.setDrawColor(0);
  }
  u8g2.drawStr(5, 44, buf);
  u8g2.setDrawColor(1);

  // Footer
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(2, 63, "4/6:ADJ  5:SET  *:EXIT");
}

static void displaySettings() {
  char buf[32];
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(2, 14, "CONFIG");
  u8g2.drawHLine(0, 16, 128);

  u8g2.setFont(u8g2_font_ncenB08_tr);

  // Warn
  sprintf(buf, "Warn: %.1f", g_warningThreshold);
  if (g_settingsIndex == 0) {
      u8g2.drawBox(0, 22, 128, 12);
      u8g2.setDrawColor(0);
  }
  u8g2.drawStr(5, 31, buf);
  u8g2.setDrawColor(1);

  // Crit
  sprintf(buf, "Crit: %.1f", g_criticalThreshold);
  if (g_settingsIndex == 1) {
      u8g2.drawBox(0, 35, 128, 12);
      u8g2.setDrawColor(0);
  }
  u8g2.drawStr(5, 44, buf);
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(2, 63, "4/6:ADJ  5:SAVE  *:EXIT");
}

static void displayAbout() {
  u8g2.setFont(u8g2_font_helvB10_tr);
  u8g2.drawStr(2, 14, "ABOUT");
  u8g2.drawHLine(0, 16, 128);

  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (g_aboutPage == 0) {
      u8g2.drawStr(5, 30, "EDGE HMI v1.0");
      u8g2.drawStr(5, 45, "Industrial Unit");
  } else {
      if (g_isWifiConnected) {
        u8g2.drawStr(5, 30, "IP Address:");
        u8g2.setCursor(5, 45);
        u8g2.print(WiFi.localIP());
      } else {
        u8g2.drawStr(5, 30, "WiFi Disconnected");
      }
  }
  
  // Page Dots
  u8g2.setFont(u8g2_font_9x15_tr); // Big dots
  u8g2.drawStr(100, 60, g_aboutPage == 0 ? ". o" : "o .");
}