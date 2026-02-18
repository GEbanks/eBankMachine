// ============================
// FILE: net.cpp
// ============================
#include "net.h"
#include "globals.h"
#include "config.h"
#include "ui.h"
#include "ota_web.h"
#include <WiFi.h>

void wifiEnsureConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    setupWebOtaOnce();
    return;
  }

  showMsg("WiFi...", "reconnecting", 0);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    server.handleClient();
    delay(50);
  }

  setupWebOtaOnce();
}
