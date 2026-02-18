// ============================
// FILE: ui.h
// ============================
#pragma once
#include <Arduino.h>

void otaDelay(unsigned long ms); // implemented in ota_web.cpp (server-friendly delay)
void showMsg(const char* line0, const char* line1 = nullptr, unsigned long ms = 0);

void showModeMenu();

void clearEntryLine();
void showEntry(const __FlashStringHelper* prompt);

void showConfirmWithdraw(long pogs);

void showDepositEnterId();
void showDepositScanning();
