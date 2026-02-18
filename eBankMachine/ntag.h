// ============================
// FILE: ntag.h
// ============================
#pragma once
#include <Arduino.h>

bool ntagRead64(char out[65]);
bool ntagWrite64(const char* data);
bool parseIdOnly(const char* data, long& outId);
