// ============================
// FILE: deposit.cpp
// ============================
#include "deposit.h"
#include "globals.h"
#include "config.h"
#include "ui.h"
#include "net.h"
#include "formbar.h"

void startDepositFlow() {
  tradeMode = MODE_REAL_TO_DIGI;
  depState = DEP_ENTER_ID;
  depToId = 0;
  depositCount = 0;
  showDepositEnterId();
}

void depositTick() {
  if (!(tradeMode == MODE_REAL_TO_DIGI && depState == DEP_SCANNING && motionState == MS_IDLE)) return;

  unsigned long nowUs = micros();
  if (nowUs - depLastSampleUs < DEP_SAMPLE_US) return;
  depLastSampleUs = nowUs;

  int v = analogRead(IR_DEP_PIN);
  bool above = (v > IR_DEP_THRESHOLD);

  unsigned long nowMs = millis();
  bool armed = (nowMs - depStartMs > 100);

  if (armed && above && !depWasAbove && nowMs >= depNextAllowedAt) {
    depositCount++;
    depNextAllowedAt = nowMs + DEP_COOLDOWN_MS;

    lcd.setCursor(7, 1);
    lcd.print("     ");
    lcd.setCursor(7, 1);
    lcd.print(depositCount);
  }
  depWasAbove = above;
}

void handleDepositKey(char k) {
  if (tradeMode != MODE_REAL_TO_DIGI) return;

  if (depState == DEP_ENTER_ID) {
    if (k == '*') {
      showDepositEnterId();
      return;
    }
    if (k >= '0' && k <= '9') {
      if (numLen < sizeof(numBuf) - 1) {
        numBuf[numLen++] = k;
        numBuf[numLen] = '\0';
        lcd.setCursor(7 + (numLen - 1), 1);
        lcd.print(k);
      }
      return;
    }
    if (k == '#') {
      long val = (numLen > 0) ? atol(numBuf) : 0;
      if (val <= 0) {
        showMsg("Invalid ID", nullptr, 900);
        showDepositEnterId();
      } else {
        depToId = val;
        depState = DEP_SCANNING;
        depositCount = 0;

        depWasAbove = false;
        depNextAllowedAt = 0;
        depStartMs = millis();
        depLastSampleUs = micros();

        showDepositScanning();
      }
      clearEntryLine();
      return;
    }
    return;
  }

  if (depState == DEP_SCANNING) {
    if (k == '#') {
      wifiEnsureConnected();
      showMsg("Sending deposit", "Please wait", 0);

      int dp = depositCount * DIGIPOGS_PER_POG_DEPOSIT;
      String resp;
      int httpc = 0;
      bool ok = formbarTransfer(KIOSK_ID, (int)depToId, dp, "Pogs -> Digi", KIOSK_ACCOUNT_PIN, resp, httpc);

      if (ok) {
        char l1[17];
        snprintf(l1, sizeof(l1), "+%d dpogs", dp);
        showMsg("Deposit OK", l1, 1800);
      } else {
        showMsg("Deposit FAIL", "Check PIN/API", 2500);
        Serial.print("Deposit HTTP=");
        Serial.println(httpc);
        Serial.println(resp);
      }

      tradeMode = MODE_SELECT;
      showModeMenu();
      return;
    }
  }
}
