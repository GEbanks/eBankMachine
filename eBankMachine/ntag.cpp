// ============================
// FILE: ntag.cpp
// ============================
#include "ntag.h"
#include "globals.h"

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
