// ============================
// FILE: formbar.h
// ============================
#pragma once
#include <Arduino.h>

bool formbarTransfer(int from, int to, int amount, const char* reason, int pin, String& outResp, int& outHttp);
bool trySendRefundNow(); // uses global refundPending state
