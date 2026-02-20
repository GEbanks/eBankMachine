#include "eBankMachine.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  dbgClear();
  dbgPrintf("BOOT\n");

  hardwareInit();

  // WiFi initial connect (15s)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    dbgPrintf("WiFi OK IP=%s\n", WiFi.localIP().toString().c_str());
    setupWebOtaOnce();
  } else {
    dbgPrintf("WiFi FAILED\n");
  }

  showModeMenu();
}

void loop() {
  otaTick();

  if (WiFi.status() != WL_CONNECTED) wifiEnsureConnected();

  refundTick();
  limitSwitchTick();

  dropTick();
  depositTick();
  cardTick();

  keypadTick();

  delay(1);
}