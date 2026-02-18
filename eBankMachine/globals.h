// ============================
// FILE: globals.h
// ============================
#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Adafruit_PN532.h>
#include <Keypad.h>

// ----- Global hardware objects (defined once in globals.cpp) -----
extern WebServer server;
extern LiquidCrystal_I2C lcd;
extern Servo myServo;
extern Adafruit_PN532 nfc;
extern Keypad keypad;

// ----- OTA server started flag -----
extern bool otaStarted;

// ----- Modes / UI -----
enum TradeMode { MODE_SELECT, MODE_DIGI_TO_REAL, MODE_REAL_TO_DIGI, MODE_UPDATE_CARD };
extern TradeMode tradeMode;

extern char     numBuf[10];
extern uint8_t  numLen;

// ----- Withdraw wizard -----
enum WizardState { WZ_ENTER_FROM, WZ_ENTER_PIN, WZ_ENTER_POGS, WZ_CONFIRM };
extern WizardState wzState;
extern long wzFrom, wzPin, wzPogs;

// ----- Deposit flow -----
enum DepositState { DEP_ENTER_ID, DEP_SCANNING };
extern DepositState depState;
extern long depToId;
extern int  depositCount;

// ----- Card update (stub kept compatible) -----
enum CardState { CARD_ENTER_ID, CARD_TAP_TO_WRITE };
extern CardState cardState;
extern long cardWriteId;
extern bool pendingCardWrite;

// ----- Limit switch debounce -----
extern int lastReading;
extern int stableState;
extern unsigned long lastChange;
extern bool limitSwitchPressed;
extern bool prevLimitSwitchPressed;

// ----- IR thresholds + state -----
extern int IR_DROP_THRESHOLD;
extern int IR_DEP_THRESHOLD;

extern unsigned long irLastSample;
extern unsigned long nextCountAllowedAt;
extern bool irWasAbove;
extern unsigned long dropStartMs;

extern bool depWasAbove;
extern unsigned long depNextAllowedAt;
extern unsigned long depStartMs;
extern unsigned long depLastSampleUs;

// ----- Drop counters -----
extern volatile int targetDrops;
extern volatile int droppedCount;

// ----- Motion -----
enum MotionState { MS_IDLE, MS_DROPPING };
extern MotionState motionState;

// ----- Refund (auto) -----
extern bool refundPending;
extern long refundToId;
extern int refundDigipogs;
extern unsigned long nextRefundTryAt;

// ----- Debug keypad helper press windows -----
extern int dPressCount;
extern unsigned long dWindowStart;
extern int cPressCount;
extern unsigned long cWindowStart;
