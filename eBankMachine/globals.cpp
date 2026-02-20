#include "eBankMachine.h"

/* Keypad layout + pins */
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

/* Debug buffer */
static char dbgBuf[4096];
static size_t dbgLen = 0;

void dbgClear() {
  dbgLen = 0;
  dbgBuf[0] = '\0';
}

void dbgAppend(const char* s) {
  if (!s) return;
  size_t sl = strlen(s);
  if (sl == 0) return;

  if (sl >= sizeof(dbgBuf)) {
    memcpy(dbgBuf, s + (sl - (sizeof(dbgBuf) - 1)), sizeof(dbgBuf) - 1);
    dbgBuf[sizeof(dbgBuf) - 1] = '\0';
    dbgLen = strlen(dbgBuf);
    return;
  }

  if (dbgLen + sl >= sizeof(dbgBuf)) {
    size_t drop = (dbgLen + sl) - (sizeof(dbgBuf) - 1);
    if (drop > dbgLen) drop = dbgLen;
    memmove(dbgBuf, dbgBuf + drop, dbgLen - drop);
    dbgLen -= drop;
    dbgBuf[dbgLen] = '\0';
  }

  memcpy(dbgBuf + dbgLen, s, sl);
  dbgLen += sl;
  dbgBuf[dbgLen] = '\0';
}

void dbgPrintf(const char* fmt, ...) {
  char tmp[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);

  Serial.print(tmp);
  dbgAppend(tmp);
}

/* expose raw buffer to web file via function-local capture in ota_web.cpp using dbgAppend/Printf.
   (we’ll serve by calling dbgAppend/Printf only; the web handler will use dbgText() below) */
static const char* dbgText() { return dbgBuf; }

/* state */
TradeMode tradeMode = MODE_SELECT;

char numBuf[10] = {0};
uint8_t numLen = 0;

WizardState wzState = WZ_ENTER_FROM;
long wzFrom = 0, wzPin = 0, wzPogs = 0;

DepositState depState = DEP_ENTER_ID;
long depToId = 0;
int depositCount = 0;

int lastReading = LOW;
int stableState = LOW;
unsigned long lastChange = 0;
bool limitSwitchPressed = false;
bool prevLimitSwitchPressed = false;

int IR_DROP_THRESHOLD = 0;
int IR_DEP_THRESHOLD  = 0;

int bPressCount = 0;
unsigned long bWindowStart = 0;

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

/* helper for ota_web.cpp to serve text */
const char* __dbgTextForWeb() { return dbgText(); }