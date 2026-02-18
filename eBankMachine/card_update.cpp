// ============================
// FILE: card_update.cpp
// ============================
// This keeps your build compiling while you re-add your card write flow.
// It’s wired into the menu + keypad, but does nothing dangerous by default.

#include "eBankMachine.h"

void startCardUpdateFlow() {
  tradeMode = MODE_UPDATE_CARD;
  cardState = CARD_ENTER_ID;
  cardWriteId = 0;
  pendingCardWrite = false;
  showEntry(F("Card: Enter ID"));
}

void cardTick() {
  // placeholder: put your NFC poll/write logic back here when ready
}

void handleCardKey(char k) {
  (void)k;
  // placeholder
}
