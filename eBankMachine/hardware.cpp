// ============================
// FILE: hardware.cpp
// ============================
#include "eBankMachine.h"

static bool servoAttached = false;

void servoAttach() {
  if (!servoAttached) {
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    servoAttached = true;
  }
}

void servoStopDetach() {
  if (servoAttached) {
    myServo.writeMicroseconds(neutral_us);
    delay(10);
    myServo.detach();
    servoAttached = false;
  }
}

static void calibrateIRPin(int pin, int& thrOut) {
  long sum = 0;
  otaDelay(200);
  for (int i = 0; i < CALIB_READS; i++) {
    sum += analogRead(pin);
    otaDelay(25);
  }
  int baseline = sum / CALIB_READS;
  thrOut = baseline + 300;
  if (thrOut > 3900) thrOut = 3900;
  if (thrOut < 0) thrOut = 0;
}

void IR_Calibration() {
  showMsg("IR Calibrating", "Keep chutes clear", 0);

  calibrateIRPin(IR_DROP_PIN, IR_DROP_THRESHOLD);
  calibrateIRPin(IR_DEP_PIN,  IR_DEP_THRESHOLD);

  irLastSample = millis();
  irWasAbove = false;

  depLastSampleUs = micros();
  depWasAbove = false;
}

void handleLimitPressed() {
  // If we were dropping, compute remaining refund
  if (motionState == MS_DROPPING) {
    int remainingPogs = (int)targetDrops - (int)droppedCount;
    if (remainingPogs < 0) remainingPogs = 0;

    int remainingDpogs = remainingPogs * DIGIPOGS_PER_POG_WITHDRAW;

    if (remainingDpogs > 0 && wzFrom > 0) {
      refundPending = true;
      refundToId = wzFrom;
      refundDigipogs = remainingDpogs;
      nextRefundTryAt = millis();
    } else {
      refundPending = false;
      refundToId = 0;
      refundDigipogs = 0;
    }

    servoStopDetach();
    motionState = MS_IDLE;
    targetDrops = 0;
    droppedCount = 0;
  }

  showMsg("LIMIT HIT", "UNJAM UP 20s", 0);
  servoAttach();
  myServo.writeMicroseconds(SERVO_UP_US);
  otaDelay(20000);
  servoStopDetach();

  wzState = WZ_ENTER_FROM;
  numLen = 0;
  numBuf[0] = '\0';

  if (refundPending) {
    showMsg("Refunding...", "Please wait", 0);
    bool sent = trySendRefundNow();

    if (sent) {
      showMsg("Refund SENT", "OK", 1500);
    } else {
      nextRefundTryAt = millis() + REFUND_RETRY_MS;
      showMsg("Refund FAILED", "Auto retry...", 1800);
    }
  } else {
    showMsg("Recovered", "Ready", 1200);
  }

  tradeMode = MODE_SELECT;
  showModeMenu();
}

void limitSwitchTick() {
  int reading = digitalRead(SWITCH_PIN);
  if (reading != lastReading) {
    lastChange = millis();
    lastReading = reading;
  }
  if (millis() - lastChange > DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    limitSwitchPressed = (ACTIVE_LOW ? (stableState == LOW) : (stableState == HIGH));
  }
  digitalWrite(LED_PIN, limitSwitchPressed ? HIGH : LOW);

  if (limitSwitchPressed && !prevLimitSwitchPressed) {
    prevLimitSwitchPressed = true;
    handleLimitPressed();
    return;
  }
  if (!limitSwitchPressed && prevLimitSwitchPressed) {
    prevLimitSwitchPressed = false;
  }
}

void hardwareInit() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(SWITCH_PIN, ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);

  lastReading = digitalRead(SWITCH_PIN);
  stableState = lastReading;
  limitSwitchPressed = (ACTIVE_LOW ? (stableState == LOW) : (stableState == HIGH));
  prevLimitSwitchPressed = limitSwitchPressed;

  keypad.setDebounceTime(15);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(40);

  lcd.init();
  lcd.backlight();
  showMsg("BOOTING...", nullptr, 500);

  // PN532
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) nfc.SAMConfig();

  IR_Calibration();

  // If switch is already pressed at boot, auto unjam once
  if (limitSwitchPressed) {
    handleLimitPressed();
  }
}
