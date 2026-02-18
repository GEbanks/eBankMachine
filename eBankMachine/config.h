// ============================
// FILE: config.h
// ============================
#pragma once
#include <Arduino.h>

// ===== WiFi + Formbar =====
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

extern const char* TRANSFER_URL;
extern const char* API_KEY;

extern const int   KIOSK_ID;
extern const int   KIOSK_ACCOUNT_PIN;

extern const int   DIGIPOGS_PER_POG_WITHDRAW;  // Digi -> Pogs
extern const int   DIGIPOGS_PER_POG_DEPOSIT;   // Pogs -> Digi

// ===== Web OTA =====
extern const char* OTA_HOST;
extern const char* OTA_PASSWORD;

// ===== I2C =====
static constexpr int SDA_PIN = 21;
static constexpr int SCL_PIN = 22;

// ===== LCD =====
static constexpr uint8_t LCD_ADDR = 0x27;

// ===== PN532 =====
static constexpr int PN532_IRQ   = -1;
static constexpr int PN532_RESET = -1;
static constexpr uint16_t PN532_TIMEOUT_MS = 100;

// ===== IR pins =====
static constexpr int IR_DROP_PIN = 34;
static constexpr int IR_DEP_PIN  = 35;

// ===== Servo =====
static constexpr int SERVO_PIN = 14;

// ===== Limit switch =====
static constexpr int  SWITCH_PIN = 23;
static constexpr bool ACTIVE_LOW = true;

// ===== LED =====
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif
static constexpr int LED_PIN = LED_BUILTIN;

// ===== Debounce / timing =====
extern const unsigned long DEBOUNCE_MS;

extern const unsigned long IR_SAMPLE_MS;
extern const unsigned long DEP_SAMPLE_US;
extern const unsigned long DROP_COOLDOWN_MS;
extern const unsigned long DEP_COOLDOWN_MS;

extern const int CALIB_READS;

extern const unsigned long D_WINDOW_MS;

extern const unsigned long NFC_POLL_MS;

extern const unsigned long REFUND_RETRY_MS;

// ===== Servo pulse widths =====
extern int neutral_us;
extern int SERVO_DOWN_US;
extern int SERVO_UP_US;
