/*
  Web OTA (browser upload) + Digipog kiosk code
  LIMIT SWITCH FIX:
    - When the limit switch is pressed:
        * Servo runs UP for 20 seconds (unjam)
        * If it was dropping, it calculates remaining pogs NOT dropped,
          converts to dpogs, and automatically refunds them to the user (wzFrom)
        * If refund fails (wifi/api), it keeps retrying automatically every 8s

  Open:
    http://<ESP32_IP>/
    login: admin / admin
    then /serverIndex

  OTA password:
    E_banks
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

/* ===================== WIFI + FORMBAR ===================== */
const char* ssid = "robonet";
const char* password = "formDog220!";

const char* TRANSFER_URL = "https://formbeta.yorktechapps.com/api/digipogs/transfer";
const char* API_KEY      = "1404409d473c2bac59b0eb8564a3dfb76b07b2d794e5de4fc0183afdf3e3b3d0";

// Kiosk account (this is your machine account)
const int KIOSK_ID = 28;

// IMPORTANT: this is the KIOSK ACCOUNT PIN (Formbar), NOT a GPIO pin
const int KIOSK_ACCOUNT_PIN = 1234;  // set this to the kiosk account PIN you use for deposits/refunds

// Conversion
const int DIGIPOGS_PER_POG_WITHDRAW = 150;  // Digi -> Pogs
const int DIGIPOGS_PER_POG_DEPOSIT  = 100;  // Pogs -> Digi

/* ===================== WEB OTA SETTINGS ===================== */
const char* host         = "digipog-kiosk";
const char* OTA_PASSWORD = "E_banks";

WebServer server(80);
bool otaStarted = false; // <-- makes sure we only start the web server once

/*
 * Login page (front-end only)
 */
const char* loginIndex =
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr><td colspan=2>"
  "<center><font size=4><b>ESP32 Login Page</b></font></center><br>"
  "</td></tr>"
  "<tr><td>Username:</td><td><input type='text' size=25 name='userid'><br></td></tr>"
  "<tr><td>Password:</td><td><input type='Password' size=25 name='pwd'><br></td></tr>"
  "<tr><td><input type='submit' onclick='check(this.form)' value='Login'></td></tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form){"
  "if(form.userid.value=='admin' && form.pwd.value=='admin'){window.open('/serverIndex')}"
  "else{alert('Error Password or Username')}}"
  "</script>";

/*
 * Server Index Page (OTA)
 */
const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<h3 style='text-align:center;'>ESP32 Web OTA Updater</h3>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form' style='text-align:center;'>"
  "<div style='margin:10px;'><input type='password' name='otapass' placeholder='OTA Password'/></div>"
  "<div style='margin:10px;'><input type='file' name='update'/></div>"
  "<div style='margin:10px;'><input type='submit' value='Update'/></div>"
  "</form>"
  "<div id='prg' style='text-align:center;'>progress: 0%</div>"
  "<div id='msg' style='text-align:center;'></div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form=$('#upload_form')[0];"
  "var data=new FormData(form);"
  "$.ajax({"
  "url:'/update',type:'POST',data:data,contentType:false,processData:false,"
  "xhr:function(){var xhr=new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress',function(evt){"
  "if(evt.lengthComputable){"
  "var per=evt.loaded/evt.total;"
  "$('#prg').html('progress: '+Math.round(per*100)+'%');"
  "}},false);return xhr;},"
  "success:function(d,s){$('#msg').html(d);},"
  "error:function(){$('#msg').html('Upload error');}"
  "});"
  "});"
  "</script>";

/* ===================== I2C (LCD + PN532) ===================== */
#define SDA_PIN 21
#define SCL_PIN 22

const uint8_t LCD_ADDR = 0x27;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// PN532 over I2C (IRQ/RESET unused)
#define PN532_IRQ   -1
#define PN532_RESET -1
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
const uint16_t PN532_TIMEOUT_MS = 100;

/* ===================== IR SENSOR PINS ===================== */
#define IR_DROP_PIN 34
#define IR_DEP_PIN  35

/* ===================== Keypad ===================== */
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 19, 18, 33, 32 };
byte colPins[COLS] = { 25, 26, 27, 13 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* ===================== Servo (continuous rotation) ===================== */
const int SERVO_PIN = 14;
Servo myServo;

int neutral_us     = 1403;
int SERVO_DOWN_US  = 1403 + 250;
int SERVO_UP_US    = 1403 - 250;

bool servoAttached = false;
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

/* ===================== Limit Switch ===================== */
const int SWITCH_PIN = 23;
const bool ACTIVE_LOW = true;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
const int LED_PIN = LED_BUILTIN;

const unsigned long DEBOUNCE_MS = 20;
int lastReading = LOW, stableState = LOW;
unsigned long lastChange = 0;
bool limitSwitchPressed = false;
bool prevLimitSwitchPressed = false;

/* ===================== IR timing ===================== */
const unsigned long IR_SAMPLE_MS      = 10;
const unsigned long DEP_SAMPLE_US     = 2000;
const unsigned long DROP_COOLDOWN_MS  = 500;
const unsigned long DEP_COOLDOWN_MS   = 150;

int IR_DROP_THRESHOLD = 0;
int IR_DEP_THRESHOLD  = 0;
const int CALIB_READS = 80;

unsigned long irLastSample = 0;
unsigned long nextCountAllowedAt = 0;
bool irWasAbove = false;
unsigned long dropStartMs = 0;

bool depWasAbove = false;
unsigned long depNextAllowedAt = 0;
unsigned long depStartMs = 0;
unsigned long depLastSampleUs = 0;

/* ===================== Drop Counters ===================== */
volatile int targetDrops = 0;
volatile int droppedCount = 0;

/* ===================== Modes / UI ===================== */
enum TradeMode { MODE_SELECT, MODE_DIGI_TO_REAL, MODE_REAL_TO_DIGI, MODE_UPDATE_CARD };
TradeMode tradeMode = MODE_SELECT;

char numBuf[10];
uint8_t numLen = 0;

enum WizardState { WZ_ENTER_FROM, WZ_ENTER_PIN, WZ_ENTER_POGS, WZ_CONFIRM };
WizardState wzState = WZ_ENTER_FROM;
long wzFrom = 0, wzPin = 0, wzPogs = 0;

enum DepositState { DEP_ENTER_ID, DEP_SCANNING };
DepositState depState = DEP_ENTER_ID;
long depToId = 0;
int depositCount = 0;

int dPressCount = 0;
unsigned long dWindowStart = 0;
const unsigned long D_WINDOW_MS = 5000;

int cPressCount = 0;
unsigned long cWindowStart = 0;

unsigned long lastNfcPoll = 0;
const unsigned long NFC_POLL_MS = 60;

enum CardState { CARD_ENTER_ID, CARD_TAP_TO_WRITE };
CardState cardState = CARD_ENTER_ID;
long cardWriteId = 0;
bool pendingCardWrite = false;

/* ===================== Refund (AUTO) ===================== */
bool refundPending = false;
long refundToId = 0;
int refundDigipogs = 0;
unsigned long nextRefundTryAt = 0;
const unsigned long REFUND_RETRY_MS = 8000;

/* ===================== OTA-friendly delay ===================== */
void otaDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    delay(1);
  }
}

/* ===================== UI helpers ===================== */
void showMsg(const char* line0, const char* line1 = nullptr, unsigned long ms = 0) {
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

void startCardUpdateFlow() {
  tradeMode = MODE_UPDATE_CARD;
  cardState = CARD_ENTER_ID;
  cardWriteId = 0;
  pendingCardWrite = false;
  showEntry(F("Card: Enter ID"));
}

/* ===================== WEB OTA setup ===================== */
bool checkOtaPassword() {
  if (!server.hasArg("otapass")) return false;
  return server.arg("otapass") == OTA_PASSWORD;
}

void setupWebOta() {
  if (!MDNS.begin(host)) {
    Serial.println("mDNS start FAILED");
  } else {
    Serial.println("mDNS started");
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });

  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on(
    "/update", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();

      if (!checkOtaPassword()) {
        if (upload.status == UPLOAD_FILE_START) Serial.println("OTA denied: bad password");
        if (Update.isRunning()) Update.abort();
        return;
      }

      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        servoStopDetach();

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        else Update.printError(Serial);
      }
    }
  );

  server.begin();
  Serial.println("Web OTA server started");
}

/* ===================== WiFi helpers ===================== */
void wifiEnsureConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  showMsg("WiFi...", "reconnecting", 0);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    server.handleClient();
    delay(50);
  }

  // Start OTA server once when WiFi comes back
  if (WiFi.status() == WL_CONNECTED && !otaStarted) {
    setupWebOta();
    otaStarted = true;
    Serial.print("HTTP: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }
}

/* ===================== Formbar Transfer ===================== */
bool formbarTransfer(int from, int to, int amount, const char* reason, int pin, String& outResp, int& outHttp) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(10000);

  if (!https.begin(client, TRANSFER_URL)) {
    outResp = "begin_failed";
    outHttp = -1;
    return false;
  }

  https.addHeader("API", API_KEY);
  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> body;
  body["from"]   = from;
  body["to"]     = to;
  body["amount"] = amount;
  body["reason"] = reason;
  body["pin"]    = pin;
  body["pool"]   = false;

  String payload;
  serializeJson(body, payload);

  outHttp = https.POST(payload);
  outResp = https.getString();
  https.end();

  if (outHttp < 200 || outHttp >= 300) return false;

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, outResp)) return false;

  return (doc["success"] | false) == true;
}

/* ===================== Refund sender ===================== */
bool trySendRefundNow() {
  if (!refundPending || refundToId <= 0 || refundDigipogs <= 0) return true;

  wifiEnsureConnected();
  if (WiFi.status() != WL_CONNECTED) return false;

  String resp;
  int httpc = 0;

  bool ok = formbarTransfer(
    KIOSK_ID,
    (int)refundToId,
    refundDigipogs,
    "refund",
    KIOSK_ACCOUNT_PIN,
    resp,
    httpc
  );

  if (ok) {
    refundPending = false;
    refundToId = 0;
    refundDigipogs = 0;
    return true;
  }

  Serial.print("Refund HTTP=");
  Serial.println(httpc);
  Serial.println(resp);
  return false;
}

/* ===================== NTAG helpers (ID only) ===================== */
bool ntagRead64(char out[65]) {
  memset(out, 0, 65);
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t page[4];
    if (!nfc.ntag2xx_ReadPage(4 + i, page)) return false;
    for (uint8_t j = 0; j < 4; j++) out[i * 4 + j] = (char)page[j];
  }
  out[64] = '\0';
  return true;
}

bool ntagWrite64(const char* data) {
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t page[4] = { 0, 0, 0, 0 };
    for (uint8_t j = 0; j < 4; j++) {
      uint8_t idx = i * 4 + j;
      char c = data[idx];
      if (c == '\0') break;
      page[j] = (uint8_t)c;
    }
    if (!nfc.ntag2xx_WritePage(4 + i, page)) return false;
  }
  return true;
}

bool parseIdOnly(const char* data, long& outId) {
  const char* p = strstr(data, "ID=");
  if (!p) return false;
  outId = atol(p + 3);
  return outId > 0;
}

/* ===================== IR Calibration ===================== */
void calibrateIRPin(int pin, int& thrOut) {
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

/* ===================== Drop logic ===================== */
enum MotionState { MS_IDLE, MS_DROPPING };
MotionState motionState = MS_IDLE;

void finishDrop(const char* why) {
  (void)why;
  servoStopDetach();
  motionState = MS_IDLE;

  char line0[16], line1[16];
  snprintf(line0, sizeof(line0), "Dropped (%d)", droppedCount);
  snprintf(line1, sizeof(line1), "pog%s", droppedCount == 1 ? "" : "s");
  showMsg(line0, line1, 1200);

  targetDrops = 0;
  droppedCount = 0;

  tradeMode = MODE_SELECT;
  showModeMenu();
}

void startDrop(int count) {
  if (count <= 0) return;
  if (motionState != MS_IDLE) return;

  if (limitSwitchPressed) {
    showMsg("Cannot drop", "Limit pressed", 900);
    return;
  }

  targetDrops = count;
  droppedCount = 0;
  nextCountAllowedAt = 0;
  irWasAbove = false;
  dropStartMs = millis();

  servoAttach();
  myServo.writeMicroseconds(SERVO_DOWN_US);
  motionState = MS_DROPPING;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Dropping: "));
  lcd.print(targetDrops);
  lcd.setCursor(0, 1);
  lcd.print(F("Done: 0/"));
  lcd.print(targetDrops);
}

/* ===================== Limit switch handler (20s UP + auto refund) ===================== */
void handleLimitPressed() {
  // If we were dropping, compute remaining refund
  if (motionState == MS_DROPPING) {
    int remainingPogs = targetDrops - droppedCount;
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

/* ===================== Flow resets ===================== */
void startWithdrawWizard() {
  tradeMode = MODE_DIGI_TO_REAL;
  wzFrom = wzPin = wzPogs = 0;
  wzState = WZ_ENTER_FROM;
  showEntry(F("Enter FROM ID"));
}

void startDepositFlow() {
  tradeMode = MODE_REAL_TO_DIGI;
  depState = DEP_ENTER_ID;
  depToId = 0;
  depositCount = 0;
  showDepositEnterId();
}

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);
  delay(200);

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

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // START OTA WEB SERVER ONCE (THIS IS THE IMPORTANT FIX)
    setupWebOta();
    otaStarted = true;

    Serial.print("HTTP: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  } else {
    Serial.println("WiFi FAILED to connect.");
  }

  // If switch is already pressed at boot, auto unjam once
  if (limitSwitchPressed) {
    handleLimitPressed();
  }

  showModeMenu();
}

/* ===================== Loop ===================== */
void loop() {
  server.handleClient();

  // WiFi keep-alive
  if (WiFi.status() != WL_CONNECTED) wifiEnsureConnected();

  // Auto-retry pending refund
  if (refundPending && motionState == MS_IDLE && millis() >= nextRefundTryAt) {
    bool sent = trySendRefundNow();
    if (sent) {
      showMsg("Refund SENT", "OK", 1200);
      showModeMenu();
    } else {
      nextRefundTryAt = millis() + REFUND_RETRY_MS;
    }
  }

  // Limit switch debounce
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

  // Detect press edge
  if (limitSwitchPressed && !prevLimitSwitchPressed) {
    prevLimitSwitchPressed = true;
    handleLimitPressed();
    return;
  }
  if (!limitSwitchPressed && prevLimitSwitchPressed) {
    prevLimitSwitchPressed = false;
  }

  // IR drop counting
  if (motionState == MS_DROPPING && !limitSwitchPressed) {
    unsigned long now = millis();
    if (now - irLastSample >= IR_SAMPLE_MS) {
      irLastSample = now;

      int v = analogRead(IR_DROP_PIN);
      bool above = (v > IR_DROP_THRESHOLD);
      bool armed = (now - dropStartMs > 200);

      if (armed && above && !irWasAbove && now >= nextCountAllowedAt) {
        droppedCount++;
        nextCountAllowedAt = now + DROP_COOLDOWN_MS;

        lcd.setCursor(6, 1);
        lcd.print(droppedCount);
        lcd.print(F("/"));
        lcd.print(targetDrops);

        if (droppedCount >= targetDrops) finishDrop("done");
      }
      irWasAbove = above;
    }
  }

  // Deposit counting (FAST)
  if (tradeMode == MODE_REAL_TO_DIGI && depState == DEP_SCANNING && motionState == MS_IDLE) {
    unsigned long nowUs = micros();
    if (nowUs - depLastSampleUs >= DEP_SAMPLE_US) {
      depLastSampleUs = nowUs;

      int v = analogRead(IR_DEP_PIN);
      bool above = (v > IR_DEP_THRESHOLD);

      unsigned long nowMs = millis();
      bool armed = (nowMs - depStartMs > 100);

      if (armed && above && !depWasAbove && nowMs >= depNextAllowedAt) {
        depositCount++;
        depNextAllowedAt = nowMs + DEP_COOLDOWN_MS;

        lcd.setCursor(7, 1);
        lcd.print("     ");
        lcd.setCursor(7, 1);
        lcd.print(depositCount);
      }
      depWasAbove = above;
    }
  }

  // Keypad handling
  if (keypad.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (!keypad.key[i].stateChanged) continue;
      if (keypad.key[i].kstate != PRESSED) continue;

      char k = keypad.key[i].kchar;
      unsigned long now = millis();

      if (tradeMode == MODE_SELECT) {
        if (k == '1') startWithdrawWizard();
        else if (k == '2') startDepositFlow();
        else if (k == '3') startCardUpdateFlow();
        continue;
      }

      if (tradeMode == MODE_DIGI_TO_REAL) {
        if (k == 'C') {
          if (motionState != MS_IDLE) { showMsg("Busy...", nullptr, 400); continue; }
          if (cPressCount == 0 || (now - cWindowStart) > D_WINDOW_MS) { cPressCount = 0; cWindowStart = now; }
          cPressCount++;
          if (cPressCount >= 3) { cPressCount = 0; cWindowStart = 0; startDrop(1); }
          continue;
        }

        if (k == 'D') {
          if (motionState != MS_IDLE) { showMsg("Busy...", nullptr, 400); continue; }
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
          continue;
        }
      }

      if (tradeMode == MODE_REAL_TO_DIGI) {
        if (depState == DEP_ENTER_ID) {
          if (k == '*') {
            showDepositEnterId();
          } else if (k >= '0' && k <= '9') {
            if (numLen < sizeof(numBuf) - 1) {
              numBuf[numLen++] = k;
              numBuf[numLen] = '\0';
              lcd.setCursor(7 + (numLen - 1), 1);
              lcd.print(k);
            }
          } else if (k == '#') {
            long val = (numLen > 0) ? atol(numBuf) : 0;
            if (val <= 0) {
              showMsg("Invalid ID", nullptr, 900);
              showDepositEnterId();
            } else {
              depToId = val;
              depState = DEP_SCANNING;
              depositCount = 0;

              depWasAbove = false;
              depNextAllowedAt = 0;
              depStartMs = millis();
              depLastSampleUs = micros();

              showDepositScanning();
            }
          }
          continue;
        }

        if (depState == DEP_SCANNING) {
          if (k == '#') {
            wifiEnsureConnected();
            showMsg("Sending deposit", "Please wait", 0);

            int dp = depositCount * DIGIPOGS_PER_POG_DEPOSIT;
            String resp;
            int httpc = 0;
            bool ok = formbarTransfer(KIOSK_ID, (int)depToId, dp, "Pogs -> Digi", KIOSK_ACCOUNT_PIN, resp, httpc);

            if (ok) {
              char l1[17];
              snprintf(l1, sizeof(l1), "+%d dpogs", dp);
              showMsg("Deposit OK", l1, 1800);
            } else {
              showMsg("Deposit FAIL", "Check PIN/API", 2500);
              Serial.print("Deposit HTTP=");
              Serial.println(httpc);
              Serial.println(resp);
            }

            tradeMode = MODE_SELECT;
            showModeMenu();
          }
          continue;
        }
      }

      if (tradeMode == MODE_UPDATE_CARD) {
        // (You can paste your full card update section back in here if you want it in this build)
        // Keeping this sketch focused on fixing OTA start + PIN masking.
      }

      if (tradeMode == MODE_DIGI_TO_REAL) {
        if (k == '*') {
          if (wzState == WZ_CONFIRM) { tradeMode = MODE_SELECT; showModeMenu(); }
          else clearEntryLine();
          continue;
        }

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
          continue;
        }

        // ===== DIGIT ENTRY: mask PIN with '*' =====
        if (k >= '0' && k <= '9') {
          if (numLen < sizeof(numBuf) - 1) {
            numBuf[numLen++] = k;
            numBuf[numLen] = '\0';

            lcd.setCursor(7 + (numLen - 1), 1);
            if (wzState == WZ_ENTER_PIN) lcd.print('*');
            else                          lcd.print(k);
          }
          continue;
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
          continue;
        }
      }
    }
  }

  delay(1);
}
