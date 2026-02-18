// ============================
// FILE: refund_tick.cpp
// ============================
#include "refund_tick.h"
#include "globals.h"
#include "config.h"
#include "formbar.h"
#include "ui.h"

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
