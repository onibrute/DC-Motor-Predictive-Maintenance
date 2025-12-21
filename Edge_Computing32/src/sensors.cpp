#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// -------- I2C buses --------
// I2C_1: 21/22 -> MPU1 @ 0x69
// I2C_2: 18/19 -> MPU2 @ 0x68
TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);

// -------- MPU instances ----
Adafruit_MPU6050 mpu1;
Adafruit_MPU6050 mpu2;

static bool mpu1_found = false;
static bool mpu2_found = false;

// -------- Offsets (din calibrare) --------
// 1 g ≈ 16384 LSB; 1 g ≈ 9.81 m/s^2 
constexpr float LSB_PER_G = 16384.0f;
constexpr float G_MS2     = 9.81f;

// MPU1 (21/22, 0x69)
constexpr float AX1_OFF_MS2 =  976.34f  / LSB_PER_G * G_MS2;
constexpr float AY1_OFF_MS2 = -2318.55f / LSB_PER_G * G_MS2;
constexpr float AZ1_OFF_MS2 =  1030.27f / LSB_PER_G * G_MS2;

// MPU2 (18/19, 0x68)
constexpr float AX2_OFF_MS2 =  1935.74f / LSB_PER_G * G_MS2;
constexpr float AY2_OFF_MS2 =   171.94f / LSB_PER_G * G_MS2;
constexpr float AZ2_OFF_MS2 =   727.05f / LSB_PER_G * G_MS2;

// ================= INIT ==================

bool sensors_init() {
  Serial.println("[SENS] === sensors_init START ===");

  // Bus 1: 21/22
  I2C_1.begin(I2C_SDA, I2C_SCL);
  I2C_1.setClock(400000);

  // Bus 2: 18/19
  I2C_2.begin(I2C2_SDA, I2C2_SCL);
  I2C_2.setClock(100000);   // lasam clar pe 100 kHz ca la sketch

  // Scan rapid pe I2C_2 ca in sketch-ul de test
  Serial.println("[SENS] Scan pe I2C_2 (18/19) in sensors_init:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    I2C_2.beginTransmission(addr);
    uint8_t err = I2C_2.endTransmission();
    if (err == 0) {
      Serial.print("  Gasit la 0x");
      Serial.println(addr, HEX);
    }
  }

  // Citire directa WHO_AM_I de la 0x68 pe busul 2
  Serial.println("[SENS] Citire directa WHO_AM_I de la 0x68 pe I2C_2...");
  I2C_2.beginTransmission(0x68);
  I2C_2.write(0x75);  // registrul WHO_AM_I
  uint8_t err = I2C_2.endTransmission(false); // fara STOP, urmeaza read
  uint8_t who = 0;
  if (err == 0) {
    I2C_2.requestFrom((uint8_t)0x68, (uint8_t)1);
    if (I2C_2.available()) {
      who = I2C_2.read();
    }
  }
  Serial.print("[SENS] WHO_AM_I brut = 0x");
  Serial.println(who, HEX);

  // --- MPU1 pe 21/22, 0x69 ---
  if (mpu1.begin(0x69, &I2C_1)) {
    mpu1_found = true;
    mpu1.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu1.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu1.setFilterBandwidth(MPU6050_BAND_184_HZ);
    Serial.println("[SENS] MPU1 (21/22,0x69) OK");
  } else {
    Serial.println("[SENS] MPU1 (21/22,0x69) ERROR!");
  }

  delay(500);  // dam timp mare inainte de MPU2

  // --- MPU2 pe 18/19, 0x68 ---
  Serial.println("[SENS] Incerc init Adafruit_MPU6050 pe 0x68 (I2C_2)...");
  bool ok2 = mpu2.begin(0x68, &I2C_2);
  Serial.print("[SENS] mpu2.begin() => ");
  Serial.println(ok2 ? "TRUE" : "FALSE");

  if (ok2) {
    mpu2_found = true;
    mpu2.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu2.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu2.setFilterBandwidth(MPU6050_BAND_184_HZ);
    Serial.println("[SENS] MPU2 (18/19,0x68) OK");
  } else {
    Serial.println("[SENS] MPU2 (18/19,0x68) NOT FOUND");
  }

  Serial.print("[SENS] Final: mpu1_found=");
  Serial.print(mpu1_found);
  Serial.print(", mpu2_found=");
  Serial.println(mpu2_found);
  Serial.println("[SENS] === sensors_init END ===");

  return mpu1_found || mpu2_found;
}




// ========== FILL FRAME (FFT + RMS pt. AMBELE) ==========

void sensors_fillFrame(double vReal1[], double vImag1[],
                       double vReal2[], double vImag2[],
                       float& rms1_out, float& rms2_out)
{
  const uint32_t Ts_us = (uint32_t)(1e6f / FS_HZ);
  uint32_t next_t = micros();

  sensors_event_t a1, g1, t1;
  sensors_event_t a2, g2, t2;

  double sumSq1 = 0.0;
  double sumSq2 = 0.0;
  int    count1 = 0;
  int    count2 = 0;

  // daca nu exista senzori, umplem cu zero
  if (!mpu1_found && !mpu2_found) {
    for (int i = 0; i < SAMPLES; i++) {
      vReal1[i] = vImag1[i] = 0.0;
      vReal2[i] = vImag2[i] = 0.0;
    }
    rms1_out = rms2_out = 0.0f;
    return;
  }

  for (int i = 0; i < SAMPLES; i++) {
    uint32_t now = micros();
    if (next_t > now) {
      delayMicroseconds(next_t - now);
    }

    // -------- MPU1: semnal + RMS1 --------
    if (mpu1_found) {
      mpu1.getEvent(&a1, &g1, &t1);

      float ax1 = a1.acceleration.x - AX1_OFF_MS2;
      float ay1 = a1.acceleration.y - AY1_OFF_MS2;
      float az1 = a1.acceleration.z - AZ1_OFF_MS2;
      float mag1 = sqrtf(ax1 * ax1 + ay1 * ay1 + az1 * az1);

      // scoatem 1 g static
      float sig1 = mag1 - G_MS2;

      vReal1[i] = (double)sig1;
      vImag1[i] = 0.0;

      sumSq1 += (double)sig1 * (double)sig1;
      count1++;
    } else {
      vReal1[i] = vImag1[i] = 0.0;
    }

    // -------- MPU2: semnal + RMS2 --------
    if (mpu2_found) {
      mpu2.getEvent(&a2, &g2, &t2);

      float ax2 = a2.acceleration.x - AX2_OFF_MS2;
      float ay2 = a2.acceleration.y - AY2_OFF_MS2;
      float az2 = a2.acceleration.z - AZ2_OFF_MS2;
      float mag2 = sqrtf(ax2 * ax2 + ay2 * ay2 + az2 * az2);

      float sig2 = mag2 - G_MS2;

      vReal2[i] = (double)sig2;
      vImag2[i] = 0.0;

      sumSq2 += (double)sig2 * (double)sig2;
      count2++;
    } else {
      vReal2[i] = vImag2[i] = 0.0;
    }

    next_t += Ts_us;
  }

  rms1_out = (count1 > 0) ? sqrtf((float)(sumSq1 / count1)) : 0.0f;
  rms2_out = (count2 > 0) ? sqrtf((float)(sumSq2 / count2)) : 0.0f;
}
