#include "firebase_sync.h"
#include "state.h"
#include "web_api.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static WiFiClientSecure fbClient;
static HTTPClient fbHttp;
static bool fbInitDone               = false;
static unsigned long lastStatusPush  = 0;
static unsigned long lastHistoryPush = 0;
static unsigned long lastCmdPoll     = 0;
static String lastHandledCmdId       = "";
static String lastPushedStatus       = "";
static bool loggedFirstSuccess       = false;

static void ensureClient() {
  if (!fbInitDone) {
    fbClient.setInsecure();
    fbHttp.setReuse(true);
    fbHttp.setTimeout(3000);
    fbInitDone = true;
  }
}

static String buildUrl(const String& path) {
  String base = String(FIREBASE_DB_URL);
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + path + ".json";
  if (strlen(FIREBASE_AUTH) > 0) {
    url += "?auth=" + String(FIREBASE_AUTH);
  }
  return url;
}

static bool firebaseSend(const char* method, const String& path, const String& jsonBody) {
  ensureClient();
  if (!fbHttp.begin(fbClient, buildUrl(path))) return false;
  fbHttp.addHeader("Content-Type", "application/json");
  int code = fbHttp.sendRequest(method, jsonBody);
  if (code < 200 || code >= 300) {
    Serial.printf("[Firebase] %s %s failed: HTTP %d (%s)\n",
                  method, path.c_str(), code, fbHttp.errorToString(code).c_str());
    fbHttp.end();
    return false;
  }
  if (!loggedFirstSuccess) {
    loggedFirstSuccess = true;
    Serial.println("[Firebase] Fast Keep-Alive sync connected (HTTP " + String(code) + ")");
  }
  fbHttp.end();
  return true;
}

static String firebaseGet(const String& path) {
  ensureClient();
  if (!fbHttp.begin(fbClient, buildUrl(path))) return "";
  int code = fbHttp.GET();
  String body = (code >= 200 && code < 300) ? fbHttp.getString() : "";
  fbHttp.end();
  return body;
}

static void firebaseDelete(const String& path) {
  ensureClient();
  if (!fbHttp.begin(fbClient, buildUrl(path))) return;
  fbHttp.sendRequest("DELETE");
  fbHttp.end();
}

static String jsonField(const String& json, const String& key) {
  String needle = "\"" + key + "\":\"";
  int i = json.indexOf(needle);
  if (i < 0) return "";
  i += needle.length();
  int j = json.indexOf("\"", i);
  if (j < 0) return "";
  return json.substring(i, j);
}

static void pushLiveStateNow() {
  String st = statusJson();
  String tk = allTicketsJson();
  String bundle = "{\"status\":" + st + ",\"tickets\":" + tk + "}";
  if (firebaseSend("PATCH", "/parking", bundle)) {
    lastPushedStatus = st;
    lastStatusPush = millis();
  }
}

void syncFirebase() {
  if (strlen(FIREBASE_DB_URL) == 0) return;
  if (apMode || WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  // 1. Poll for incoming remote commands every 700 ms (fast over Keep-Alive)
  if (now - lastCmdPoll >= 700) {
    lastCmdPoll = now;
    String raw = firebaseGet("/parking/command");
    if (raw.length() >= 5 && raw != "null") {
      String id     = jsonField(raw, "id");
      String action = jsonField(raw, "action");
      String arg1   = jsonField(raw, "arg1");
      String arg2   = jsonField(raw, "arg2");

      if (id.length() > 0 && id != lastHandledCmdId) {
        lastHandledCmdId = id;
        String result = executeRemoteCommand(action, arg1, arg2);
        firebaseSend("PUT", "/parking/responses/" + id, result);
        firebaseDelete("/parking/command");
        // Immediately push updated status & tickets so Vercel updates right away
        pushLiveStateNow();
        return;
      }
    }
  }

  // 2. Push status + tickets immediately on change (max 1/s), or every 5s heartbeat
  if (now - lastStatusPush >= 1000) {
    String cur = statusJson();
    if (cur != lastPushedStatus || now - lastStatusPush >= 5000) {
      String bundle = "{\"status\":" + cur + ",\"tickets\":" + allTicketsJson() + "}";
      if (firebaseSend("PATCH", "/parking", bundle)) {
        lastPushedStatus = cur;
        lastStatusPush = now;
      }
    }
  }

  // 3. Push occupancy history & alerts every 25 seconds in one PATCH
  if (lastHistoryPush == 0 || now - lastHistoryPush >= 25000) {
    lastHistoryPush = now;
    String meta = "{\"history\":" + historyJson() + ",\"alerts\":" + alertsJson() + "}";
    firebaseSend("PATCH", "/parking", meta);
  }
}
