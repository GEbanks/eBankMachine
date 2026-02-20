#include "eBankMachine.h"

void keypadTick() {
  if (!keypad.getKeys()) return;

  for (int i = 0; i < LIST_MAX; i++) {
    if (!keypad.key[i].stateChanged) continue;
    if (keypad.key[i].kstate != PRESSED) continue;

    char k = keypad.key[i].kchar;

    unsigned long now = millis();

// B x3 -> show IP
if (k == 'B') {
  if (bPressCount == 0 || (now - bWindowStart) > D_WINDOW_MS) {
    bPressCount = 0;
    bWindowStart = now;
  }

  bPressCount++;

  if (bPressCount >= 3) {
    bPressCount = 0;
    bWindowStart = 0;

    if (WiFi.status() == WL_CONNECTED) {
      String ip = WiFi.localIP().toString();
      showMsg("IP Address:", ip.c_str(), 3000);
    } else {
      showMsg("WiFi Not", "Connected", 2000);
    }

    // Return to previous mode screen
    if (tradeMode == MODE_SELECT) showModeMenu();
  }

  return;
}

    if (tradeMode == MODE_SELECT) {
      if (k == '1') startWithdrawWizard();
      else if (k == '2') startDepositFlow();
      else if (k == '3') startCardUpdateFlow();
      continue;
    }

    if (tradeMode == MODE_DIGI_TO_REAL) {
      handleWithdrawKey(k);
      continue;
    }

    if (tradeMode == MODE_REAL_TO_DIGI) {
      handleDepositKey(k);
      continue;
    }

    if (tradeMode == MODE_UPDATE_CARD) {
      handleCardKey(k);
      continue;
    }
  }
}