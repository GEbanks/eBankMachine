#include "eBankMachine.h"

const char* fbErrMsg(FbErr e) {
  switch (e) {
    case FB_NO_WIFI:    return "Network issue";
    case FB_BEGIN_FAIL: return "Network issue";
    case FB_POST_FAIL:  return "Network issue";
    case FB_HTTP_FAIL:  return "Server issue";
    case FB_JSON_FAIL:  return "Bad reply";
    case FB_API_REJECT: return "PIN/ID/balance";
    default:            return "OK";
  }
}

bool formbarTransferEx(int from, int to, int amount, const char* reason, int pin,
                       String& outResp, int& outHttp, FbErr& outErr) {
  outResp = "";
  outHttp = 0;
  outErr  = FB_OK;

  if (WiFi.status() != WL_CONNECTED) {
    outErr = FB_NO_WIFI;
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(10000);

  if (!https.begin(client, TRANSFER_URL)) {
    outResp = "begin_failed";
    outHttp = -1;
    outErr  = FB_BEGIN_FAIL;
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

  // <=0 = connection failure / timeout / could not reach
  if (outHttp <= 0) {
    outErr = FB_POST_FAIL;
    return false;
  }

  if (outHttp < 200 || outHttp >= 300) {
    outErr = FB_HTTP_FAIL;
    return false;
  }

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, outResp)) {
    outErr = FB_JSON_FAIL;
    return false;
  }

  bool success = (doc["success"] | false);
  if (!success) {
    outErr = FB_API_REJECT;
    return false;
  }

  outErr = FB_OK;
  return true;
}

// OPTIONAL: keep old signature so you don't have to update everything at once.
// This will still work, but you lose the specific reason unless you use Ex().
bool formbarTransfer(int from, int to, int amount, const char* reason, int pin, String& outResp, int& outHttp) {
  FbErr err;
  return formbarTransferEx(from, to, amount, reason, pin, outResp, outHttp, err);
}

bool trySendRefundNow() {
  if (!refundPending || refundToId <= 0 || refundDigipogs <= 0) return true;

  wifiEnsureConnected();
  if (WiFi.status() != WL_CONNECTED) return false;

  String resp;
  int httpc = 0;
  FbErr err;

  bool ok = formbarTransferEx(
    KIOSK_ID,
    (int)refundToId,
    refundDigipogs,
    "refund",
    KIOSK_ACCOUNT_PIN,
    resp,
    httpc,
    err
  );

  if (ok) {
    refundPending = false;
    refundToId = 0;
    refundDigipogs = 0;
    dbgPrintf("Refund OK\n");
    return true;
  }

  dbgPrintf("Refund FAIL err=%d http=%d resp=%s\n", (int)err, httpc, resp.c_str());
  return false;
}