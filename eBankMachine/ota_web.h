// ============================
// FILE: ota_web.h
// ============================
#pragma once
#include <Arduino.h>

void setupWebOtaOnce(); // safe to call repeatedly
void otaTick();         // runs server.handleClient()
void otaDelay(unsigned long ms); // delay that services HTTP
