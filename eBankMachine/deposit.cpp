// ============================
// deposit.cpp
// ============================
#include "eBankMachine.h"

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

  // Enter ID mode
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

  // Scanning mode: # sends deposit
  if (depState == DEP_SCANNING && k == '#') {
    wifiEnsureConnected();
    showMsg("Sending deposit", "Please wait", 0);

    int dp = depositCount * DIGIPOGS_PER_POG_DEPOSIT;
    String resp;
    int httpc = 0;
    FbErr err;

    bool ok = formbarTransferEx(
      KIOSK_ID,
      (int)depToId,
      dp,
      "Pogs -> Digi",
      KIOSK_ACCOUNT_PIN,
      resp,
      httpc,
      err
    );

    if (ok) {
      char l1[17];
      snprintf(l1, sizeof(l1), "+%d dpogs", dp);
      showMsg("Deposit OK", l1, 1800);
      dbgPrintf("Deposit OK to=%ld dp=%d\n", depToId, dp);
    } else {
      showMsg("Deposit FAIL", fbErrMsg(err), 2500);
      dbgPrintf("Deposit FAIL err=%d http=%d resp=%s\n", (int)err, httpc, resp.c_str());
    }

    tradeMode = MODE_SELECT;
    showModeMenu();
  }
}