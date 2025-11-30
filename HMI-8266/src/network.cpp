#include "network.h"
#include "config.h"
#include "state.h"
#include "types.h"
#include "settings.h"
#include "utils.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

static WiFiUDP udp;
static char packetBuffer[512];

// ===== ARMA SECRETĂ: IP SWEEPER =====
// Dacă routerul nu ne lasă să strigăm la toți, îi luăm la rând pe fiecare!
// În HMI / src / network.cpp

static bool resolveServerIP() {
  if (!g_isWifiConnected) return false;
  if (g_serverIp != IPAddress(0,0,0,0)) return true;

  IPAddress local = WiFi.localIP();
  
  Serial.println("[NET] Mod Ninja Activat: Scanez rețeaua în liniște...");

  // Scanăm de la .1 la .254
  for (int i = 1; i < 255; i++) {
    // Sărim peste adresa noastră și gateway (.1) ca să nu pierdem timp
    if (i == local[3] || i == 1) continue;

    IPAddress target(local[0], local[1], local[2], i);
    
    udp.beginPacket(target, UDP_PORT);
    udp.print("PING_DISCOVERY"); 
    udp.endPacket();
    
    // === TRUCUL STEALTH ===
    // Așteptăm 15ms între pachete. 
    // Routerul se relaxează și nu ne blochează.
    delay(15); 
    
    // Feedback vizual la fiecare 20 de adrese, să știi că mișcă
    if (i % 20 == 0) Serial.print(".");
    
    // Verificăm rapid dacă a răspuns cineva ÎN TIMP ce scanăm
    if (udp.parsePacket()) {
       // Dacă am primit ceva, oprim scanarea imediat!
       return false; // Se va ocupa loop-ul principal de citire
    }
  }
  
  Serial.println("\n[NET] Scanare completă. Aștept răspuns...");
  return false; 
}

// ===== Init =====
void network_init() {
  Serial.println("[NET] Pornire Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G); 
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500); Serial.print("."); attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    g_isWifiConnected = true;
    Serial.print("[NET] Conectat! IP: "); Serial.println(WiFi.localIP());
    udp.begin(UDP_PORT);
    
    // Declanșăm scanarea imediat
    resolveServerIP();
  } else {
    g_isWifiConnected = false;
  }
}

// ===== Loop =====
void network_loop() {
  if (WiFi.status() != WL_CONNECTED) {
    g_isWifiConnected = false;
    static unsigned long lastRec = 0;
    if (millis() - lastRec > 10000) { WiFi.reconnect(); lastRec = millis(); }
    return;
  }
  g_isWifiConnected = true;

  // Heartbeat / Scanare la 5 secunde (dacă am pierdut conexiunea)
  static unsigned long lastPing = 0;
  if (millis() - lastPing > 5000) {
    if (g_serverIp == IPAddress(0,0,0,0)) {
        resolveServerIP(); // LANSEAZĂ MITRALIERA
    } else {
        network_sendCommand("ping"); // Doar un ping politicos dacă știm unde e
    }
    lastPing = millis();
  }

  // --- RECEPTIE DATE ---
  int packetSize = udp.parsePacket();
  if (packetSize) {
    
    // VICTORIE: Dacă primim ceva și nu știam cine e, tocmai l-am găsit!
    if (g_serverIp == IPAddress(0,0,0,0)) {
       g_serverIp = udp.remoteIP();
       Serial.print("[NET] TINTA DOBORATA! ESP32 gasit la: "); 
       Serial.println(g_serverIp);
    }

    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = 0;
      if (packetBuffer[0] == '{') {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, packetBuffer);

          if (!error) {
            if (doc["m1"]) {
                g_motor1.speed = doc["m1"]["spd"].as<int>();
                g_motor1.state = doc["m1"]["on"].as<bool>(); 
                g_motor1.rms   = doc["m1"]["rms"].as<float>();
                g_motor1.frequency = doc["m1"]["frq"].as<float>();
                g_motor1.crestFactor = doc["m1"]["cst"].as<float>();
            }
            if (doc["m2"]) {
                g_motor2.speed = doc["m2"]["spd"].as<int>();
                g_motor2.state = doc["m2"]["on"].as<bool>();
            }
          }
      }
    }
  }
}

void network_sendCommand(const String& cmd) {
  if (!g_isWifiConnected) return;
  // Dacă nu știm unde e, lansăm scanarea din nou
  if (g_serverIp == IPAddress(0,0,0,0)) { resolveServerIP(); return; }

  udp.beginPacket(g_serverIp, UDP_PORT);
  udp.print(cmd);
  udp.endPacket();
}

bool network_isConnected() { return g_isWifiConnected; }
IPAddress network_getServerIP() { return g_serverIp; }