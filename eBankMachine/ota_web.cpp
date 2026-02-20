#include "eBankMachine.h"

extern const char* __dbgTextForWeb(); // from globals.cpp

static const char* loginIndex =
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

static const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<h3 style='text-align:center;'>ESP32 Web OTA Updater</h3>"
  "<div style='text-align:center; margin-bottom:10px;'>"
  "<a href='/debug' target='_blank'>Open Debug Log</a>"
  "</div>"
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

static bool checkOtaPassword() {
  if (!server.hasArg("otapass")) return false;
  return server.arg("otapass") == OTA_PASSWORD;
}

void otaDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    delay(1);
  }
}

void setupWebOtaOnce() {
  if (otaStarted) return;
  if (WiFi.status() != WL_CONNECTED) return;

  if (!MDNS.begin(OTA_HOST)) {
    dbgPrintf("mDNS start FAILED\n");
  } else {
    dbgPrintf("mDNS started\n");
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });

  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  // ===== Debug endpoints (same port 80) =====
  server.on("/debug.txt", HTTP_GET, []() {
    server.send(200, "text/plain", __dbgTextForWeb());
  });

  server.on("/debug/clear", HTTP_POST, []() {
    dbgClear();
    server.send(200, "text/plain", "cleared");
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "rebooting");
    delay(150);
    ESP.restart();
  });

  server.on("/debug", HTTP_GET, []() {
    String page =
      "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'/>"
      "<title>eBank Debug</title></head>"
      "<body style='font-family:monospace;'>"
      "<h3>Debug Log</h3>"
      "<form method='POST' action='/debug/clear' style='display:inline;'>"
      "<button type='submit'>Clear</button></form> "
      "<form method='POST' action='/reboot' style='display:inline;'>"
      "<button type='submit'>Reboot</button></form>"
      "<pre id='log' style='white-space:pre-wrap;'></pre>"
      "<script>"
      "function tick(){fetch('/debug.txt').then(r=>r.text()).then(t=>{"
      "document.getElementById('log').innerText=t;"
      "window.scrollTo(0,document.body.scrollHeight);"
      "});}"
      "setInterval(tick,1000); tick();"
      "</script>"
      "</body></html>";
    server.send(200, "text/html", page);
  });

  // ===== OTA upload endpoint =====
  server.on(
    "/update", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      delay(150);
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();

      if (!checkOtaPassword()) {
        if (upload.status == UPLOAD_FILE_START) dbgPrintf("OTA denied: bad password\n");
        if (Update.isRunning()) Update.abort();
        return;
      }

      if (upload.status == UPLOAD_FILE_START) {
        dbgPrintf("Update: %s\n", upload.filename.c_str());
        servoStopDetach();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) dbgPrintf("Update Success: %u\nRebooting...\n", (unsigned)upload.totalSize);
        else Update.printError(Serial);
      }
    }
  );

  server.begin();
  otaStarted = true;

  dbgPrintf("HTTP: http://%s/\n", WiFi.localIP().toString().c_str());
  dbgPrintf("Debug: http://%s/debug\n", WiFi.localIP().toString().c_str());
}

void otaTick() {
  server.handleClient();
}