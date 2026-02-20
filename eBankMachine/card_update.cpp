#include "eBankMachine.h"

void startCardUpdateFlow() {
  tradeMode = MODE_UPDATE_CARD;
  showMsg("Card mode", "stub", 800);
  tradeMode = MODE_SELECT;
  showModeMenu();
}

void cardTick() {
  // stub
}

void handleCardKey(char k) {
  (void)k;
  // stub
}