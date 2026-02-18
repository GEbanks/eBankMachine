// ============================
// FILE: config.cpp
// ============================
#include "config.h"

// ===== WiFi + Formbar =====
const char* WIFI_SSID = "robonet";
const char* WIFI_PASS = "formDog220!";

const char* TRANSFER_URL = "https://formbeta.yorktechapps.com/api/digipogs/transfer";
const char* API_KEY      = "1404409d473c2bac59b0eb8564a3dfb76b07b2d794e5de4fc0183afdf3e3b3d0";

const int KIOSK_ID = 28;
const int KIOSK_ACCOUNT_PIN = 1234;

const int DIGIPOGS_PER_POG_WITHDRAW = 150;
const int DIGIPOGS_PER_POG_DEPOSIT  = 100;

// ===== Web OTA =====
const char* OTA_HOST     = "digipog-kiosk";
const char* OTA_PASSWORD = "E_banks";

// ===== Timing =====
const unsigned long DEBOUNCE_MS = 20;

const unsigned long IR_SAMPLE_MS     = 10;
const unsigned long DEP_SAMPLE_US    = 2000;
const unsigned long DROP_COOLDOWN_MS = 500;
const unsigned long DEP_COOLDOWN_MS  = 150;

const int CALIB_READS = 80;

const unsigned long D_WINDOW_MS   = 5000;
const unsigned long NFC_POLL_MS   = 60;
const unsigned long REFUND_RETRY_MS = 8000;

// ===== Servo =====
int neutral_us    = 1403;
int SERVO_DOWN_US = 1403 + 250;
int SERVO_UP_US   = 1403 - 250;
