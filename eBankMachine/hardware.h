// ============================
// FILE: hardware.h
// ============================
#pragma once

void hardwareInit();

void servoAttach();
void servoStopDetach();

void IR_Calibration();

void limitSwitchTick(); // debounce + edge detect + calls handler

// handler when limit pressed
void handleLimitPressed();
