#include "firebase_sync.h"
#include "state.h"
#include "web_api.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static unsigned long lastStatusPush  = 0;
static unsigned long lastHistoryPush = 0;
static unsigned long lastCmdPoll     = 0;
static String lastHandledCmdId       = "";

static String buildUrl(const String& path) {
  String base = String(FIREBASE_DB_URL);
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + path + ".json";
  if (strlen(FIREBASE_AUTH) > 0) {
    url += "?auth=" + String(FIREBASE_AUTH);
  }
  return url;
}

static bool firebasePut(const String& path, const String& jsonBody) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(3500);
  if (!http.begin(client, buildUrl(path))) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(jsonBody);
  http.end();
  return code >= 200 && code < 300;
}

static String firebaseGet(const String& path) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(3500);
  if (!http.begin(client, buildUrl(path))) return "";
  int code = http.GET();
  String body = (code >= 200 && code < 300) ? http.getString() : "";
  http.end();
  return body;
}

static void firebaseDelete(const String& path) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(3000);
  if (!http.begin(client, buildUrl(path))) return;
  http.sendRequest("DELETE");
  http.end();
}

// Extract a flat JSON string field like "action":"pay"
static String jsonField(const String& json, const String& key) {
  String needle = "\"" + key + "\":\"";
  int i = json.indexOf(needle);
  if (i < 0) return "";
  i += needle.length();
  int j = json.indexOf("\"", i);
  if (j < 0) return "";
  return json.substring(i, j);
}

void syncFirebase() {
  if (strlen(FIREBASE_DB_URL) == 0) return;
  if (apMode || WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  // 1. Push live status + tickets + heartbeat every 2 seconds
  if (now - lastStatusPush >= 2000) {
    lastStatusPush = now;
    String payload = statusJson();
    // Inject server-side heartbeat timestamp before closing brace
    if (payload.endsWith("}")) {
      payload.remove(payload.length() - 1);
      payload += ",\"fbSyncMs\":" + String(now) + "}";
    }
    firebasePut("/parking/status", payload);
    firebasePut("/parking/tickets", allTicketsJson());
  }

  // 2. Push occupancy history & alerts every 15 seconds
  if (now - lastHistoryPush >= 15000) {
    lastHistoryPush = now;
    firebasePut("/parking/history", historyJson());
    firebasePut("/parking/alerts", alertsJson());
  }

  // 3. Poll for a pending remote command from Vercel every 1.5 seconds
  if (now - lastCmdPoll >= 1500) {
    lastCmdPoll = now;
    String raw = firebaseGet("/parking/command");
    if (raw.length() < 5 || raw == "null") return;

    String id     = jsonField(raw, "id");
    String action = jsonField(raw, "action");
    String arg1   = jsonField(raw, "arg1");
    String arg2   = jsonField(raw, "arg2");

    if (id.length() == 0 || id == lastHandledCmdId) return;
    lastHandledCmdId = id;

    String result = executeRemoteCommand(action, arg1, arg2);
    firebasePut("/parking/responses/" + id, result);
    firebaseDelete("/parking/command");
  }
}
