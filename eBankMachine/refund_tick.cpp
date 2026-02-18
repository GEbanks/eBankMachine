// ============================
// FILE: refund_tick.cpp
// ============================
#include "eBankMachine.h"

void refundTick() {
  if (!refundPending) return;
  if (motionState != MS_IDLE) return;
  if (millis() < nextRefundTryAt) return;

  bool sent = trySendRefundNow();
  if (sent) {
    showMsg("Refund SENT", "OK", 1200);
    showModeMenu();
  } else {
    nextRefundTryAt = millis() + REFUND_RETRY_MS;
  }
}
