#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "utils.h"
#include "sensors.h"
#include "motors.h"
#include "network.h"

// Buffere Motor 1 (MPU1)
static double vReal1[SAMPLES];
static double vImag1[SAMPLES];
static float  lastFFT1[SAMPLES/2];
static FrameMetrics fm1;
static float  lastRMS1 = 0.0f;

// Buffere Motor 2 (MPU2)
static double vReal2[SAMPLES];
static double vImag2[SAMPLES];
static float  lastFFT2[SAMPLES/2];
static FrameMetrics fm2;
static float  lastRMS2 = 0.0f;


// Mutex
static SemaphoreHandle_t mtxMetrics;
static SemaphoreHandle_t mtxFFT;

static void sensorTask(void* pv) {
  const TickType_t frameTicks = pdMS_TO_TICKS(FRAME_MS);

  for (;;) {
    float rms1 = 0.0f;
    float rms2 = 0.0f;

    // 1. Citim ambii senzori -> umplem bufferele si obtinem RMS pe fiecare
    sensors_fillFrame(vReal1, vImag1,
                      vReal2, vImag2,
                      rms1, rms2);

    // 2. Calculăm FFT + metrici pentru fiecare motor
    FrameMetrics tmp1;
    FrameMetrics tmp2;
    signal_computeMetrics(vReal1, vImag1, tmp1, FS_HZ, SAMPLES);
    signal_computeMetrics(vReal2, vImag2, tmp2, FS_HZ, SAMPLES);

    // 3. Salvăm datele (protejate de mtxMetrics)
    if (xSemaphoreTake(mtxMetrics, pdMS_TO_TICKS(5)) == pdTRUE) {
      fm1      = tmp1;
      fm2      = tmp2;
      lastRMS1 = rms1;
      lastRMS2 = rms2;
      xSemaphoreGive(mtxMetrics);
    }

    // 4. Salvăm FFT-urile (magnitudinea e deja in vReal1/vReal2) [attached_file:137]
    if (xSemaphoreTake(mtxFFT, pdMS_TO_TICKS(5)) == pdTRUE) {
      for (int k = 0; k < SAMPLES/2; k++) {
        lastFFT1[k] = (float)vReal1[k];
        lastFFT2[k] = (float)vReal2[k];
      }
      xSemaphoreGive(mtxFFT);
    }

    // 5. Asteptam pana la urmatorul frame
    vTaskDelay(frameTicks);
  }
}


static void netTask(void* pv) {
  const TickType_t loopDelay = pdMS_TO_TICKS(50);
  uint32_t lastWs = 0;

  for (;;) {
    // logica periodică de rețea (UDP, MQTT, WS)
    network_loop();

    uint32_t now = millis();
    if (now - lastWs >= 200) {   // Trimite date la ~5 Hz
      lastWs = now;

      FrameMetrics snap1, snap2;
      float fftSnap1[SAMPLES/2];
      float fftSnap2[SAMPLES/2];
      float rms1Snap = 0.0f;
      float rms2Snap = 0.0f;

      // Luăm o copie a metricalor (protejate de mtxMetrics)
      if (xSemaphoreTake(mtxMetrics, pdMS_TO_TICKS(5)) == pdTRUE) {
        snap1   = fm1;
        snap2   = fm2;
        rms1Snap = lastRMS1;
        rms2Snap = lastRMS2;
        xSemaphoreGive(mtxMetrics);
      }

      // Luăm o copie a FFT-urilor (protejate de mtxFFT)
      if (xSemaphoreTake(mtxFFT, pdMS_TO_TICKS(5)) == pdTRUE) {
        for (int k = 0; k < SAMPLES/2; k++) {
          fftSnap1[k] = lastFFT1[k];
          fftSnap2[k] = lastFFT2[k];
        }
        xSemaphoreGive(mtxFFT);
      }

      // Trimitem totul la rețea (M1 + M2)
      network_publish(snap1, snap2,
                      fftSnap1, fftSnap2, SAMPLES/2,
                      rms1Snap, rms2Snap);
    }

    vTaskDelay(loopDelay);
  }
}
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== BUILD NOU PDM ===");
  Serial.println(__DATE__ " " __TIME__);

  
  mtxMetrics = xSemaphoreCreateMutex();
  mtxFFT     = xSemaphoreCreateMutex();

  // Init Module
  if (!sensors_init()) Serial.println("[ERR] Sensors Failed!");
  else Serial.println("[OK] Sensors Ready");

  motors_init();
  network_init();

  // Pornim Task-urile Dual-Core
  xTaskCreatePinnedToCore(sensorTask, "SensTask", 8192, NULL, 2, NULL, 1); // Core 1
  xTaskCreatePinnedToCore(netTask,    "NetTask",  8192, NULL, 1, NULL, 0); // Core 0

  Serial.println("System Running.");
}

void loop() {
  // Gol, totul e în task-uri
}