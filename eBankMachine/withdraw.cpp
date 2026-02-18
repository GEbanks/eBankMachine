// ============================
// FILE: withdraw.cpp
// ============================
#include "withdraw.h"
#include "globals.h"
#include "config.h"
#include "ui.h"
#include "net.h"
#include "formbar.h"
#include "drop.h"
#include "hardware.h"

void startWithdrawWizard() {
  tradeMode = MODE_DIGI_TO_REAL;
  wzFrom = wzPin = wzPogs = 0;
  wzState = WZ_ENTER_FROM;
  showEntry(F("Enter FROM ID"));
}

// NOTE: includes the “C triple drop 1” and “D triple unjam” shortcuts exactly like your original
void handleWithdrawKey(char k) {
  if (tradeMode != MODE_DIGI_TO_REAL) return;

  unsigned long now = millis();

  // Debug / service keys
  if (k == 'C') {
    if (motionState != MS_IDLE) { showMsg("Busy...", nullptr, 400); return; }
    if (cPressCount == 0 || (now - cWindowStart) > D_WINDOW_MS) { cPressCount = 0; cWindowStart = now; }
    cPressCount++;
    if (cPressCount >= 3) { cPressCount = 0; cWindowStart = 0; startDrop(1); }
    return;
  }

  if (k == 'D') {
    if (motionState != MS_IDLE) { showMsg("Busy...", nullptr, 400); return; }
    if (dPressCount == 0 || (now - dWindowStart) > D_WINDOW_MS) { dPressCount = 0; dWindowStart = now; }
    dPressCount++;
    if (dPressCount >= 3) {
      dPressCount = 0; dWindowStart = 0;
      showMsg("UNJAM UP", nullptr, 300);
      servoAttach();
      myServo.writeMicroseconds(SERVO_UP_US);
      otaDelay(2000);
      servoStopDetach();
      showModeMenu();
      tradeMode = MODE_SELECT;
    }
    return;
  }

  // Cancel / clear
  if (k == '*') {
    if (wzState == WZ_CONFIRM) { tradeMode = MODE_SELECT; showModeMenu(); }
    else clearEntryLine();
    return;
  }

  // Confirm screen
  if (wzState == WZ_CONFIRM) {
    if (k == '#') {
      wifiEnsureConnected();
      showMsg("Transferring...", "Please wait", 0);

      int digipogs = (int)wzPogs * DIGIPOGS_PER_POG_WITHDRAW;
      String resp;
      int httpc = 0;
      bool ok = formbarTransfer((int)wzFrom, KIOSK_ID, digipogs, "Digi -> Pogs", (int)wzPin, resp, httpc);

      if (ok) {
        showMsg("Transfer OK", "Dropping...", 700);
        startDrop((int)wzPogs);
      } else {
        showMsg("Transfer FAIL", "Bad PIN/ID?", 2200);
        Serial.print("Withdraw HTTP=");
        Serial.println(httpc);
        Serial.println(resp);
        tradeMode = MODE_SELECT;
        showModeMenu();
      }
    }
    return;
  }

  // Digit entry (mask PIN)
  if (k >= '0' && k <= '9') {
    if (numLen < sizeof(numBuf) - 1) {
      numBuf[numLen++] = k;
      numBuf[numLen] = '\0';

      lcd.setCursor(7 + (numLen - 1), 1);
      if (wzState == WZ_ENTER_PIN) lcd.print('*');
      else                        lcd.print(k);
    }
    return;
  }

  if (k == '#') {
    long val = (numLen > 0) ? atol(numBuf) : 0;

    if (wzState == WZ_ENTER_FROM) {
      if (val <= 0) { showMsg("Invalid FROM", nullptr, 900); showEntry(F("Enter FROM ID")); }
      else { wzFrom = val; wzState = WZ_ENTER_PIN; showEntry(F("Enter PIN")); }
    } else if (wzState == WZ_ENTER_PIN) {
      if (val <= 0) { showMsg("Invalid PIN", nullptr, 900); showEntry(F("Enter PIN")); }
      else { wzPin = val; wzState = WZ_ENTER_POGS; showEntry(F("Enter POGS")); }
    } else if (wzState == WZ_ENTER_POGS) {
      if (val <= 0) { showMsg("Invalid POGS", nullptr, 900); showEntry(F("Enter POGS")); }
      else { wzPogs = val; wzState = WZ_CONFIRM; showConfirmWithdraw(wzPogs); }
    }

    clearEntryLine();
    return;
  }
}
