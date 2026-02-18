// ============================
// FILE: keypad_router.cpp
// ============================
#include "keypad_router.h"
#include "globals.h"
#include "ui.h"
#include "withdraw.h"
#include "deposit.h"
#include "card_update.h"

void keypadTick() {
  if (!keypad.getKeys()) return;

  for (int i = 0; i < LIST_MAX; i++) {
    if (!keypad.key[i].stateChanged) continue;
    if (keypad.key[i].kstate != PRESSED) continue;

    char k = keypad.key[i].kchar;

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
