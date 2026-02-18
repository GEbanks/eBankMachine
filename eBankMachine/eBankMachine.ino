// ============================
// eBankMachine.ino
// ============================
#include <WiFi.h>

#include "config.h"
#include "globals.h"

#include "ota_web.h"
#include "net.h"
#include "hardware.h"
#include "ui.h"

#include "drop.h"
#include "deposit.h"
#include "withdraw.h"
#include "card_update.h"

#include "keypad_router.h"
#include "refund_tick.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  hardwareInit();

  // WiFi initial connect (15s) + then start OTA server once connected
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    setupWebOtaOnce();
  } else {
    Serial.println("WiFi FAILED to connect.");
  }

  showModeMenu();
}

void loop() {
  // OTA HTTP server tick
  otaTick();

  // WiFi keep-alive (also re-starts OTA when WiFi returns)
  if (WiFi.status() != WL_CONNECTED) wifiEnsureConnected();

  // auto refund retries
  refundTick();

  // limit switch debounce + edge handling
  limitSwitchTick();

  // drop IR counting
  dropTick();

  // deposit IR counting
  depositTick();

  // card tick (stub)
  cardTick();

  // keypad -> routes to correct mode handler
  keypadTick();

  delay(1);
}
