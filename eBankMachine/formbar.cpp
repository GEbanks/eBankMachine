// ============================
// FILE: formbar.cpp
// ============================
#include "formbar.h"
#include "globals.h"
#include "config.h"
#include "net.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

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
