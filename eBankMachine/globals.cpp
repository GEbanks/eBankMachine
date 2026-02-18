// ============================
// FILE: globals.cpp
// ============================
#include "globals.h"
#include "config.h"

// Keypad layout + pins
static const byte ROWS = 4, COLS = 4;
static char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
static byte rowPins[ROWS] = { 19, 18, 33, 32 };
static byte colPins[COLS] = { 25, 26, 27, 13 };

WebServer server(80);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Servo myServo;
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool otaStarted = false;

TradeMode tradeMode = MODE_SELECT;

char numBuf[10] = {0};
uint8_t numLen = 0;

WizardState wzState = WZ_ENTER_FROM;
long wzFrom = 0, wzPin = 0, wzPogs = 0;

DepositState depState = DEP_ENTER_ID;
long depToId = 0;
int depositCount = 0;

CardState cardState = CARD_ENTER_ID;
long cardWriteId = 0;
bool pendingCardWrite = false;

int lastReading = LOW;
int stableState = LOW;
unsigned long lastChange = 0;
bool limitSwitchPressed = false;
bool prevLimitSwitchPressed = false;

int IR_DROP_THRESHOLD = 0;
int IR_DEP_THRESHOLD  = 0;

unsigned long irLastSample = 0;
unsigned long nextCountAllowedAt = 0;
bool irWasAbove = false;
unsigned long dropStartMs = 0;

bool depWasAbove = false;
unsigned long depNextAllowedAt = 0;
unsigned long depStartMs = 0;
unsigned long depLastSampleUs = 0;

volatile int targetDrops = 0;
volatile int droppedCount = 0;

MotionState motionState = MS_IDLE;

bool refundPending = false;
long refundToId = 0;
int refundDigipogs = 0;
unsigned long nextRefundTryAt = 0;

int dPressCount = 0;
unsigned long dWindowStart = 0;
int cPressCount = 0;
unsigned long cWindowStart = 0;
