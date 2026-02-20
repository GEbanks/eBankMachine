#include "eBankMachine.h"

void showMsg(const char* line0, const char* line1, unsigned long ms) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line0);
  if (line1) {
    lcd.setCursor(0, 1);
    lcd.print(line1);
  }
  if (ms) otaDelay(ms);
}

void showModeMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("1:Digi->Pogs"));
  lcd.setCursor(0, 1);
  lcd.print(F("2:Pogs->Digi 3"));
}

void clearEntryLine() {
  lcd.setCursor(7, 1);
  for (int j = 0; j < 9; j++) lcd.print(' ');
  lcd.setCursor(7, 1);
  numLen = 0;
  numBuf[0] = '\0';
}

void showEntry(const __FlashStringHelper* prompt) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(prompt);
  lcd.setCursor(0, 1);
  lcd.print(F("Enter:       "));
  lcd.setCursor(7, 1);
  clearEntryLine();
}

void showConfirmWithdraw(long pogs) {
  long digipogs = pogs * DIGIPOGS_PER_POG_WITHDRAW;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Use "));
  lcd.print(digipogs);
  lcd.print(F(" dpogs?"));
  lcd.setCursor(0, 1);
  lcd.print(F("*=No   #=Yes"));
}

void showDepositEnterId() { showEntry(F("Enter ID")); }

void showDepositScanning() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Deposit mode"));
  lcd.setCursor(0, 1);
  lcd.print(F("Count: 0  #=done"));
}