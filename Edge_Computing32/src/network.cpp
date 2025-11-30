#include "network.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h> // Necesar pentru descoperire
#include "motors.h"
#include "utils.h"

// ================= GLOBALS =================
static AsyncWebServer server(HTTP_PORT);
static AsyncWebSocket ws("/ws");

static WiFiClient espClient;
static PubSubClient mqtt(espClient);
static WiFiUDP udp;

// HMI Tracking (Reține adresa IP a ecranului)
static IPAddress hmiIP(0, 0, 0, 0);
static bool hmiKnown = false;

// Stare internă
static int last_m1_pwm = 0;
static int last_m2_pwm = 0;
static FrameMetrics _lastFm;
static bool isAPMode = false;

// =====================================================
//  Wi-Fi INIT (STA + fallback AP)
// =====================================================
static void startWiFi() {
  WiFi.disconnect(true, true);    
  delay(200);
  WiFi.mode(WIFI_OFF);
  delay(200);

  Serial.println("[WiFi] Starting connection attempt...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  
  // CRITICAL: Setăm Hostname pentru ca HMI să ne poată găsi
  WiFi.setHostname(HOSTNAME);
  
  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    Serial.printf("[WiFi] CONNECTED IP=%s RSSI=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    
    // Start mDNS Responder
    if (MDNS.begin(HOSTNAME)) {
      Serial.println("[mDNS] Responder started: " + String(HOSTNAME));
      // Adăugăm serviciul UDP pentru scanere
      MDNS.addService("udp", "arduino", UDP_PORT);
    }
    return;
  }

  // Dacă eșuează, pornește AP
  Serial.println("[WiFi] Connect failed. Starting AP...");
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(config.ap_ssid.c_str(), config.ap_pass.c_str());
  isAPMode = true;

  Serial.printf("[WiFi] AP %s IP=%s\n",
                ok ? "OK" : "FAIL",
                WiFi.softAPIP().toString().c_str());
}

// =====================================================
// MQTT RECONNECT
// =====================================================
static void mqttReconnect() {
  if (mqtt.connected()) return;
  // Încercăm conectarea doar în mod Station
  if (isAPMode) return; 

  Serial.print("[MQTT] Connecting...");
  if (mqtt.connect(MQTT_CLIENT)) Serial.println("OK");
  else Serial.printf("FAIL rc=%d\n", mqtt.state());
}

// =====================================================
// UDP LOGIC (COMUNICARE HMI)
// =====================================================
static void udpBegin() {
  if (udp.begin(UDP_PORT)) {
    Serial.printf("[UDP] Listening on port %u\n", UDP_PORT);
  } else {
    Serial.println("[UDP] begin() FAILED");
  }
}

static void udpLoop() {
  int psize = udp.parsePacket();
  if (psize <= 0) return;

  // === DEBUG: Să vedem cine ne caută ===
  IPAddress sender = udp.remoteIP();
  int senderPort = udp.remotePort();

  Serial.println();
  Serial.println(">>> [UDP] PACHET PRIMIT! <<<");
  Serial.print("    De la IP: "); Serial.println(sender);
  Serial.print("    Pe Port: "); Serial.println(senderPort);

  // 1. Auto-Discovery: Salvăm adresa HMI-ului ca să știm unde răspundem
  hmiIP = sender;
  hmiKnown = true;

  // 2. Citire Mesaj
  static char buf[512];
  int n = udp.read((uint8_t*)buf, sizeof(buf) - 1);
  if (n <= 0) return;
  buf[n] = 0; // Terminație șir

  String s(buf); 
  s.trim(); 
  
  Serial.print("    Continut: ["); Serial.print(s); Serial.println("]");

  // 3. Executare Comenzi
  if (s.startsWith("A=")) {
    int val = s.substring(2).toInt();
    last_m1_pwm = constrain(val, 0, 255);
    motorA_set(last_m1_pwm);
    Serial.printf("    => ACTIUNE: Motor 1 setat la %d\n", last_m1_pwm);
  }
  else if (s.startsWith("B=")) {
    int val = s.substring(2).toInt();
    last_m2_pwm = constrain(val, 0, 255);
    motorB_set(last_m2_pwm);
    Serial.printf("    => ACTIUNE: Motor 2 setat la %d\n", last_m2_pwm);
  }
  else if (s.startsWith("STOP") || s.startsWith("ESTOP")) {
    motors_stop_all();
    last_m1_pwm = 0;
    last_m2_pwm = 0;
    Serial.println("    => ACTIUNE: STOP DE URGENTA!");
  }
  else if (s == "PING_DISCOVERY") {
    Serial.println("    => ACTIUNE: Discovery primit. Voi incepe sa trimit date!");
  }
  
  Serial.println("--------------------------------");
}

// =====================================================
// WEBSOCKET EVENTS (Web Dashboard)
// =====================================================
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Client %u connected\n", (unsigned)client->id());
    return;
  }

  if (type == WS_EVT_DATA) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) { Serial.printf("[WS] JSON error: %s\n", err.c_str()); return; }

    const char* cmd = doc["cmd"] | "";
    int val = doc["val"] | 0;

    if (!strcmp(cmd, "toggleM1")) {
      int current = motorA_get_pwm();
      if (current > 0) { last_m1_pwm = 0; motorA_set(0); }
      else { if(last_m1_pwm == 0) last_m1_pwm = 180; motorA_set(last_m1_pwm); }
    }
    else if (!strcmp(cmd, "toggleM2")) {
      int current = motorB_get_pwm();
      if (current > 0) { last_m2_pwm = 0; motorB_set(0); }
      else { if(last_m2_pwm == 0) last_m2_pwm = 180; motorB_set(last_m2_pwm); }
    }
    else if (!strcmp(cmd, "set_m1_speed")) {
      last_m1_pwm = constrain(val, 0, 255);
      motorA_set(last_m1_pwm);
    }
    else if (!strcmp(cmd, "set_m2_speed")) {
      last_m2_pwm = constrain(val, 0, 255);
      motorB_set(last_m2_pwm);
    }
    else if (!strcmp(cmd, "ESTOP")) {
      motors_stop_all();
    }
    else if (!strcmp(cmd, "set_profile")) {
      motors_applyProfile(val);
      last_m1_pwm = motorA_get_pwm();
      last_m2_pwm = motorB_get_pwm();
    }
  }
}

// =====================================================
// NETWORK INITIALIZATION
// =====================================================
bool network_init() {
  Serial.println("[NET] Initializing...");

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] Mount failed");
  }

  loadConfig(); 
  startWiFi(); 
  delay(50);

  // --- Web Server Routes ---
  DefaultHeaders::Instance().addHeader("Cache-Control", "max-age=60, public");
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest* req){
    JsonDocument doc;
    doc["rms"] = _lastFm.rms;
    doc["peak"] = _lastFm.peak;
    doc["health"] = _lastFm.health;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

   // --- Settings page ---
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
    <!DOCTYPE html><html><head><title>Settings</title><meta name='viewport' content='width=device-width, initial-scale=1'><meta charset="UTF-8">
    <style>body{font-family:Arial, sans-serif;background:#1a202c;color:#e2e8f0;padding:20px;} 
    .container{max-width:500px;margin:auto;background:#2d3748;padding:30px;border-radius:15px;}
    input{width:100%;padding:10px;margin-bottom:20px;border-radius:5px;border:none;background:#4a5568;color:white;}
    input[type=submit]{background:#4CAF50;color:#1a202c;cursor:pointer;}</style>
    </head><body><div class='container'><h1>Network Settings</h1>
    <form action='/save' method='POST'>
    <label>WiFi SSID:</label><input type='text' name='wifi_ssid' value='%WIFI_SSID%'>
    <label>WiFi Password:</label><input type='password' name='wifi_pass' value='%WIFI_PASS%'>
    <label>Access Point SSID:</label><input type='text' name='ap_ssid' value='%AP_SSID%'>
    <label>Access Point Password:</label><input type='password' name='ap_pass' value='%AP_PASS%'>
    <input type='submit' value='Save and Restart'>
    </form></div></body></html>)rawliteral";

    html.replace("%WIFI_SSID%", config.wifi_ssid);
    html.replace("%WIFI_PASS%", config.wifi_pass);
    html.replace("%AP_SSID%", config.ap_ssid);
    html.replace("%AP_PASS%", config.ap_pass);

    request->send(200, "text/html", html);
  });

  // Save Config
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    SystemConfig newCfg = config;
    if (request->hasParam("wifi_ssid", true)) newCfg.wifi_ssid = request->getParam("wifi_ssid", true)->value();
    if (request->hasParam("wifi_pass", true)) newCfg.wifi_pass = request->getParam("wifi_pass", true)->value();
    saveConfig(newCfg);
    request->send(200, "text/plain", "Saved. Restarting...");
    delay(1000);
    ESP.restart();
  });

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  
  // Start UDP for HMI
  udpBegin();

  return true;
}

// =====================================================
// NETWORK MAIN LOOP
// =====================================================
void network_loop() {
  ws.cleanupClients();
  
  // Verificăm pachete UDP (Aici prindem IP-ul HMI)
  udpLoop(); 

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
  }
}

// =====================================================
// PUBLISH TELEMETRY (HMI + Web + MQTT)
// =====================================================
void network_publish(const FrameMetrics& fm1, const FrameMetrics& fm2,
                     const float* fft1, const float* fft2, int fft_len,
                     float rms1, float rms2)
{
  // 1. Rate limiting (max 5 Hz)
  static unsigned long lastSent = 0;
  unsigned long now = millis();
  if (now - lastSent < 200) return;
  lastSent = now;

  // 2. PWM curent
  int m1_pwm = last_m1_pwm;
  int m2_pwm = last_m2_pwm;

  // 3. RPM estimat
  int m1_rpm = map(m1_pwm, 0, 255, 0, 3000);
  int m2_rpm = map(m2_pwm, 0, 255, 0, 3000);

  // -------- A. HMI (UDP / OLED) --------
  if (hmiKnown) {
    JsonDocument hmiJson;

    // Motor 1
    hmiJson["m1"]["spd"] = m1_rpm;
    hmiJson["m1"]["on"]  = (m1_pwm > 0);
    hmiJson["m1"]["rms"] = rms1;             // sau fm1.rms
    hmiJson["m1"]["frq"] = fm1.dom_freq_hz;
    hmiJson["m1"]["cst"] = fm1.crest;

    // Motor 2
    hmiJson["m2"]["spd"] = m2_rpm;
    hmiJson["m2"]["on"]  = (m2_pwm > 0);
    hmiJson["m2"]["rms"] = rms2;             // sau fm2.rms
    hmiJson["m2"]["frq"] = fm2.dom_freq_hz;
    hmiJson["m2"]["cst"] = fm2.crest;

    char udpBuf[512];
    serializeJson(hmiJson, udpBuf);
    udp.beginPacket(hmiIP, UDP_PORT);
    udp.write((const uint8_t*)udpBuf, strlen(udpBuf));
    udp.endPacket();
  }

  // -------- B. WebSocket (Dashboard) --------
if (ws.count() > 0) {
  JsonDocument jd;
  jd["type"] = "telemetry";

  // Motor 1 - date complete
  jd["rms1"]        = rms1;
  jd["peak1"]       = fm1.peak;
  jd["crest1"]      = fm1.crest;
  jd["dom_f1"]      = fm1.dom_freq_hz;
  jd["health1"]     = fm1.health;
  jd["m1_speed"]    = m1_pwm;
  jd["motor1State"] = (m1_pwm > 0);

  // Motor 2 - date complete
  jd["rms2"]        = rms2;
  jd["peak2"]       = fm2.peak;
  jd["crest2"]      = fm2.crest;
  jd["dom_f2"]      = fm2.dom_freq_hz;
  jd["health2"]     = fm2.health;
  jd["m2_speed"]    = m2_pwm;
  jd["motor2State"] = (m2_pwm > 0);

  // Health global (worst-case între cele două)
  float health_min = min(fm1.health, fm2.health);
  jd["health"] = health_min;

  // Alertă bazată pe RMS-ul cel mai rău
  float rms_max = max(rms1, rms2);
  int alert = 0;
  if (rms_max >= RMS_CRIT) alert = 2;
  else if (rms_max >= RMS_WARN) alert = 1;
  jd["alertState"] = alert;

  // FFT Motor 1
  JsonArray arr1 = jd["fft1"].to<JsonArray>();
  for (int i = 0; i < fft_len; i++) arr1.add(fft1[i]);

  // FFT Motor 2
  JsonArray arr2 = jd["fft2"].to<JsonArray>();
  for (int i = 0; i < fft_len; i++) arr2.add(fft2[i]);

  String out;
  serializeJson(jd, out);
  ws.textAll(out);
}


  // -------- C. MQTT --------
if (mqtt.connected()) {
  JsonDocument md;
  
  // Motor 1
  md["rms1"]   = rms1;
  md["peak1"]  = fm1.peak;
  md["crest1"] = fm1.crest;
  md["dom_f1"] = fm1.dom_freq_hz;
  md["health1"]= fm1.health;
  md["m1"]     = m1_rpm;

  // Motor 2
  md["rms2"]   = rms2;
  md["peak2"]  = fm2.peak;
  md["crest2"] = fm2.crest;
  md["dom_f2"] = fm2.dom_freq_hz;
  md["health2"]= fm2.health;
  md["m2"]     = m2_rpm;

  // Downsample FFT1 pentru MQTT (payload mic)
  JsonArray fft64 = md["fft64"].to<JsonArray>();
  int limit = min(fft_len, 64);
  for (int i = 0; i < limit; i++) fft64.add(fft1[i]);

  char mqttBuf[2048];
  size_t n = serializeJson(md, mqttBuf, sizeof(mqttBuf));
  mqtt.publish(MQTT_TOPIC_TELE, mqttBuf, n);
}


  // Actualizăm metrica globală pentru /metrics (luăm Motor 1 ca referință)
  _lastFm = fm1;
}
