#include "web_api.h"
#include "web_pages.h"
#include "state.h"
#include "hardware.h"
#include "parking.h"

static void reply(bool ok, String msg) {
  server.send(200, "application/json",
              "{\"ok\":" + String(ok ? "true" : "false") + ",\"msg\":\"" + msg + "\"}");
}

static bool adminOk() {
  if (!server.authenticate("admin", ADMIN_PASS)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

static String statusJson() {
  int fr = freeSpaces();
  if (fr < 0) fr = 0;
  String w = "[";
  bool first = true;
  auto add = [&](String s) {
    if (!first) w += ",";
    w += "\"" + s + "\"";
    first = false;
  };
  if (entryStuck) add("Entry sensor stuck?");
  if (exitStuck) add("Exit sensor stuck?");
  if (!apMode && WiFi.RSSI() < -80) add("Weak Wi-Fi signal");
  if (ESP.getFreeHeap() < 30000) add("Low memory");
  w += "]";

  String j = "{";
  j += "\"cars\":" + String(carCount) + ",\"max\":" + String(maxCars);
  j += ",\"free\":" + String(fr) + ",\"reserved\":" + String(reservedCount());
  j += ",\"mode\":\"" + String(secureMode ? "secure" : "simple") + "\"";
  j += ",\"manual\":" + String(manualMode ? "true" : "false");
  j += ",\"muted\":" + String(muted ? "true" : "false");
  j += ",\"gate\":\"" + String(gateIsOpen ? "Open" : "Closed") + "\"";
  j += ",\"entry\":" + String(entrySeen() ? "true" : "false");
  j += ",\"exit\":" + String(exitSeen() ? "true" : "false");
  j += ",\"d1\":" + String(lastD1, 1) + ",\"d2\":" + String(lastD2, 1);
  j += ",\"rssi\":" + String(apMode ? 0 : WiFi.RSSI());
  j += ",\"ap\":" + String(apMode ? "true" : "false");
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"in\":" + String(entriesToday) + ",\"out\":" + String(exitsToday);
  j += ",\"peak\":" + String(peakCars);
  j += ",\"unpaid\":" + String(millis() < blinkUntil ? "true" : "false");
  j += ",\"alert\":" + String(millis() < alertFlashUntil ? "true" : "false");
  j += ",\"warnNow\":" + String(warnActive ? "true" : "false");
  j += ",\"tail\":" + String(millis() < tailUntil ? "true" : "false");
  j += ",\"warns\":" + w;
  j += ",\"locked\":0";
  j += ",\"events\":[";
  int lo = evCount > 12 ? evCount - 12 : 0;
  for (int i = evCount - 1; i >= lo; i--) {
    j += "{\"t\":\"" + stamp(events[i]) + "\",\"m\":\"" + events[i].text + "\"}";
    if (i > lo) j += ",";
  }
  j += "]}";
  return j;
}

static void handleHistory() {
  String j = "{\"max\":" + String(maxCars) + ",\"step\":20,\"h\":[";
  for (int i = 0; i < hCount; i++) {
    j += String(hist[i]);
    if (i < hCount - 1) j += ",";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

static void handlePay() {
  uint32_t ip = (uint32_t)server.client().remoteIP();
  long cd = cooldownLeft(ip);
  if (cd > 0) { reply(false, "Please wait " + String(cd) + " s before paying again"); return; }
  if (freeSpaces() <= 0) { reply(false, "Lot full, try again later"); return; }
  int slot = -1;
  for (int i = 0; i < MAXRES; i++) if (slotFree(res[i].st)) { slot = i; break; }
  if (slot < 0) { reply(false, "Too many open tickets"); return; }
  String m = server.arg("method");
  m.replace("\"", "");
  if (m.length() > 16) m = m.substring(0, 16);
  ticketNo++;
  paidTotal++;
  saveStats();
  res[slot].st = R_RES;
  res[slot].no = ticketNo;
  res[slot].created = millis();
  res[slot].armedAt = 0;
  res[slot].exArmAt = 0;
  res[slot].wrong = 0;
  res[slot].totalWrong = 0;
  res[slot].lockLevel = 0;
  res[slot].lockedUntil = 0;
  res[slot].ip = ip;
  String nm = server.arg("name");
  String clean = "";
  for (unsigned int k = 0; k < nm.length() && clean.length() < 14; k++) {
    char ch = nm[k];
    if (isalnum(ch) || ch == ' ' || ch == '-') clean += ch;
  }
  clean.trim();
  res[slot].name = clean.length() ? clean : String("Car");
  res[slot].token = String(esp_random(), HEX) + String(esp_random(), HEX);
  addEvent("Payment RM2 (" + m + ", demo): " + ticketName(ticketNo) + " " + res[slot].name + " reserved");
  server.send(200, "application/json",
              "{\"ok\":true,\"token\":\"" + res[slot].token +
              "\",\"msg\":\"Paid. Space reserved for 1 minute.\"}");
}

static void handleArm() {
  int i = findToken(server.arg("t"));
  if (i < 0) { reply(false, "Unknown ticket"); return; }
  if (res[i].st == R_ARMED) {
    res[i].armedAt = millis();
    reply(true, "Gate armed. Drive to the entry sensor now.");
    return;
  }
  if (res[i].st != R_RES) { reply(false, "Ticket is not ready"); return; }
  for (int k = 0; k < MAXRES; k++)
    if (res[k].st == R_ARMED) { reply(false, "Another driver is at the gate"); return; }
  res[i].st = R_ARMED;
  res[i].armedAt = millis();
  gentleUntil = 0;
  addEvent("Driver tapped I'm at the gate: " + ticketName(res[i].no));
  reply(true, "Gate armed. Drive to the entry sensor now.");
}

static void handleCancel() {
  int i = findToken(server.arg("t"));
  if (i < 0 || (res[i].st != R_RES && res[i].st != R_ARMED)) { reply(false, "Nothing to cancel"); return; }
  res[i].st = R_CANCEL;
  refundTotal++;
  saveStats();
  addEvent("Reservation cancelled (demo refund): " + ticketName(res[i].no));
  reply(true, "Cancelled. Payment refunded (demo).");
}

static void handleCode() {
  int i = findToken(server.arg("t"));
  String c = server.arg("c");
  if (i < 0) {
    if (millis() - badWin > 60000) { badWin = millis(); badTicketless = 0; }
    badTicketless++;
    if (badTicketless == 8) addAlert("Many code attempts without a valid ticket", 3);
    reply(false, "No valid ticket");
    return;
  }
  Res& t = res[i];
  if (millis() < t.lockedUntil) {
    reply(false, "Locked. Try again in " + String((t.lockedUntil - millis()) / 1000 + 1) + " s");
    return;
  }
  bool codeLive = code.length() > 0 && millis() - codeAt < CODE_MS;
  bool good = t.st == R_ARMED && codeFor == i && codeLive && c == code;
  if (good) {
    codeOk = true;
    t.wrong = 0;
    reply(true, "Code accepted. Drive in.");
    return;
  }
  if (t.st != R_ARMED) { reply(false, "Tap I'm at the gate first"); return; }
  if (codeFor != i || code.length() == 0) { reply(false, "No code yet: wait at the entry sensor"); return; }
  if (!codeLive) { reply(false, "Code expired: drive to the sensor again"); return; }

  t.wrong++;
  t.totalWrong++;
  wrongCodes++;
  if (t.totalWrong >= 6) {
    t.st = R_CANCEL;
    refundTotal++;
    saveStats();
    clearCode();
    setCooldown(t.ip);
    blinkUntil = millis() + 3000;
    startBeeps(3);
    addAlert("Ticket " + ticketName(t.no) + " blocked: too many wrong codes (demo refund)", 4);
    reply(false, "Too many wrong codes. Ticket cancelled and refunded (demo)");
    return;
  }
  if (t.wrong >= 3) {
    t.lockLevel++;
    unsigned long dur = lockMs(t.lockLevel);
    t.lockedUntil = millis() + dur;
    t.wrong = 0;
    blinkUntil = millis() + 3000;
    startBeeps(3);
    lcdMsgUntil = 0;
    showScreen("LOCKED");
    lcdMsgUntil = millis() + 3000;
    addAlert("Ticket " + ticketName(t.no) + " locked " + String(dur / 1000) + " s: 3 wrong codes", 2);
    reply(false, "Too many tries. Locked for " + String(dur / 1000) + " s");
    return;
  }
  reply(false, "Wrong code (" + String(3 - t.wrong) + " tries left)");
}

static void handleTickets() {
  String list = server.arg("t");
  String j = "[";
  bool first = true;
  int start = 0;
  while (start <= (int)list.length()) {
    int comma = list.indexOf(',', start);
    if (comma < 0) comma = list.length();
    String tk = list.substring(start, comma);
    start = comma + 1;
    int i = findToken(tk);
    if (i < 0) continue;
    Res& t = res[i];
    String st = "none";
    long left = 0, total = 1, lk = 0;
    bool need = false;
    if (millis() < t.lockedUntil) lk = (t.lockedUntil - millis()) / 1000 + 1;
    switch (t.st) {
      case R_RES:
        st = "reserved"; total = RESERVE_MS / 1000;
        left = ((long)RESERVE_MS - (long)(millis() - t.created)) / 1000;
        break;
      case R_ARMED:
        st = "armed"; total = ARM_MS / 1000;
        left = ((long)ARM_MS - (long)(millis() - t.armedAt)) / 1000;
        need = secureMode && codeFor == i && code.length() > 0 && millis() - codeAt < CODE_MS;
        break;
      case R_ENTER: st = "entering"; break;
      case R_PARKED: st = "parked"; break;
      case R_EXARM:
        st = "exiting"; total = ARM_MS / 1000;
        left = ((long)ARM_MS - (long)(millis() - t.exArmAt)) / 1000;
        break;
      case R_LEFT: st = "left"; break;
      case R_USED: st = "used"; break;
      case R_EXP: st = "expired"; break;
      case R_CANCEL: st = "cancelled"; break;
      default: break;
    }
    if (left < 0) left = 0;
    if (!first) j += ",";
    first = false;
    j += "{\"t\":\"" + tk + "\",\"n\":\"" + t.name + "\",\"ticket\":\"" + ticketName(t.no) +
         "\",\"state\":\"" + st + "\",\"left\":" + String(left) + ",\"total\":" + String(total) +
         ",\"locked\":" + String(lk) + ",\"atgate\":" + String(atGateNow() ? "true" : "false") +
         ",\"needCode\":" + String(need ? "true" : "false") + "}";
  }
  j += "]";
  server.send(200, "application/json", j);
}

static void handleExitArm() {
  int i = findToken(server.arg("t"));
  if (i < 0) { reply(false, "Unknown ticket"); return; }
  if (res[i].st == R_EXARM) {
    res[i].exArmAt = millis();
    reply(true, "Exit armed. Drive to the exit sensor now.");
    return;
  }
  if (res[i].st != R_PARKED) { reply(false, "This car is not parked"); return; }
  for (int k = 0; k < MAXRES; k++)
    if (res[k].st == R_EXARM) { reply(false, "Another car is leaving. Try again in a moment"); return; }
  res[i].st = R_EXARM;
  res[i].exArmAt = millis();
  addEvent("Driver tapped I'm exiting: " + ticketName(res[i].no) + " (" + res[i].name + ")");
  reply(true, "Exit armed. Drive to the exit sensor now.");
}

static void handleAdmin() {
  if (!adminOk()) return;
  String cmd = server.arg("cmd");
  int v = server.arg("v").toInt();
  if (cmd == "manual") {
    manualMode = (v == 1);
    if (manualMode) {
      state = IDLE;
      if (curRes >= 0) {
        res[curRes].st = R_RES;
        res[curRes].created = millis();
        curRes = -1;
      }
    }
    addEvent(manualMode ? "Admin: manual mode ON" : "Admin: manual mode OFF");
    reply(true, manualMode ? "Manual mode on. Gate is yours." : "Manual mode off. Automatic again.");
  } else if (cmd == "gate") {
    if (!manualMode) { reply(false, "Turn manual mode on first"); return; }
    pendingGate = (v == 1) ? GATE_OPEN : GATE_CLOSED;
    addEvent(v == 1 ? "Admin: gate opened" : "Admin: gate closed");
    reply(true, v == 1 ? "Opening gate..." : "Closing gate...");
  } else if (cmd == "reset") {
    carCount = 0;
    saveStats();
    addEvent("Admin: car count reset to 0");
    reply(true, "Car count reset to 0");
  } else if (cmd == "clearstats") {
    entriesToday = 0; exitsToday = 0; peakCars = carCount;
    paidTotal = 0; refundTotal = 0;
    saveStats();
    addEvent("Admin: statistics cleared");
    reply(true, "Statistics cleared");
  } else if (cmd == "clearalerts") {
    for (int i = 0; i < 6; i++) ac[i] = 0;
    wrongCodes = 0;
    alCount = 0;
    addEvent("Admin: alerts cleared");
    reply(true, "Alerts cleared");
  } else if (cmd == "cleartail") {
    tailAlarm = false;
    blinkUntil = 0;
    addEvent("Admin: tailgating alarm cleared");
    reply(true, "Tailgating alarm cleared");
  } else if (cmd == "approveexit") {
    adminExitUntil = millis() + 30000;
    addEvent("Admin: exit approved for the next car (30 s)");
    reply(true, "Next car may leave within 30 s");
  } else if (cmd == "mode") {
    secureMode = (v == 1);
    prefs.putBool("sec", secureMode);
    addEvent(secureMode ? "Admin: secure mode on (LCD code)" : "Admin: simple mode on");
    reply(true, secureMode ? "Secure mode on" : "Simple mode on");
  } else if (cmd == "mute") {
    muted = (v == 1);
    prefs.putBool("mute", muted);
    addEvent(muted ? "Admin: buzzer muted" : "Admin: buzzer on");
    reply(true, muted ? "Buzzer muted" : "Buzzer on");
  } else if (cmd == "cap") {
    if (v < 1 || v > 12) { reply(false, "Capacity must be 1 to 12"); return; }
    maxCars = v;
    prefs.putInt("max", maxCars);
    addEvent("Admin: capacity set to " + String(maxCars));
    reply(true, "Capacity is now " + String(maxCars));
  } else if (cmd == "buzz") {
    bool was = muted;
    muted = false;
    startBeeps(2);
    reply(true, was ? "Buzzer test (it is muted, this one sounds once)" : "Buzzer test");
    muted = was;
  } else if (cmd == "sim") {
    if (v < 1 || v > 3) { reply(false, "Unknown demo"); return; }
    simCmd = v;
    reply(true, "Running demo...");
  } else if (cmd == "restart") {
    carCount = 0; entriesToday = 0; exitsToday = 0; peakCars = 0;
    paidTotal = 0; refundTotal = 0; ticketNo = 0;
    for (int i = 0; i < MAXRES; i++) res[i] = Res();
    evCount = 0; alCount = 0; hCount = 0; wrongCodes = 0;
    for (int i = 0; i < 6; i++) ac[i] = 0;
    for (int i = 0; i < 4; i++) { cdIp[i] = 0; cdUntil[i] = 0; }
    clearCode();
    state = IDLE; curRes = -1; manualMode = false;
    curExit = -1; adminExitUntil = 0;
    tailAlarm = false; tailExtra = 0;
    blinkUntil = 0; gentleUntil = 0; alertFlashUntil = 0; tailUntil = 0;
    lastCount = -1;
    saveStats();
    pendingGate = GATE_CLOSED;
    addEvent("Admin: everything restarted");
    reply(true, "Everything restarted to the beginning");
  } else if (cmd == "demo") {
    demoCmd = 1;
    reply(true, "Full demo started. About 45 seconds. Do not use real cars.");
  } else {
    reply(false, "Unknown command");
  }
}

static void handleAlerts() {
  if (!adminOk()) return;
  String j = "{\"c\":[";
  for (int i = 0; i < 6; i++) {
    j += String(ac[i]);
    if (i < 5) j += ",";
  }
  j += "],\"wrong\":" + String(wrongCodes) + ",\"paid\":" + String(paidTotal) +
       ",\"ref\":" + String(refundTotal) + ",\"list\":[";
  for (int i = alCount - 1; i >= 0; i--) {
    j += "{\"t\":\"" + stamp(alerts[i]) + "\",\"m\":\"" + alerts[i].text + "\"}";
    if (i > 0) j += ",";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

static void handleCsv() {
  if (!adminOk()) return;
  String s = "time,event\n";
  for (int i = 0; i < evCount; i++) {
    String m = events[i].text;
    m.replace(",", ";");
    s += stamp(events[i]) + "," + m + "\n";
  }
  s += "\nSummary,value\n";
  s += "Cars now," + String(carCount) + "\n";
  s += "Capacity," + String(maxCars) + "\n";
  s += "Cars entered," + String(entriesToday) + "\n";
  s += "Cars left," + String(exitsToday) + "\n";
  s += "Peak cars at once," + String(peakCars) + "\n";
  s += "Payments (demo)," + String(paidTotal) + "\n";
  s += "Refunds (demo)," + String(refundTotal) + "\n";
  s += "Demo revenue (RM)," + String((paidTotal - refundTotal) * 2) + "\n";
  s += "Unpaid cars at gate," + String(ac[0]) + "\n";
  s += "Paid cars not confirmed," + String(ac[1]) + "\n";
  s += "Ticket locks," + String(ac[2]) + "\n";
  s += "Invalid code attempts," + String(ac[3]) + "\n";
  s += "Tickets blocked," + String(ac[4]) + "\n";
  s += "Tailgating warnings," + String(ac[5]) + "\n";
  s += "Wrong codes entered," + String(wrongCodes) + "\n";
  server.sendHeader("Content-Disposition", "attachment; filename=parking-log.csv");
  server.send(200, "text/csv", s);
}

static void handleAdminPage() {
  if (!adminOk()) return;
  server.send_P(200, "text/html", ADMIN_PAGE);
}

static void handleAbout() {
  String p = String(FPSTR(ABOUT_PAGE));
  p.replace("%TITLE%", PROJECT_TITLE);
  p.replace("%NAME%", STUDENT_NAME);
  p.replace("%SUP%", SUPERVISOR);
  server.send(200, "text/html", p);
}

void registerRoutes() {
  server.on("/", []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", PAGE);
  });
  server.on("/admin", handleAdminPage);
  server.on("/about", handleAbout);
  server.on("/download.csv", handleCsv);
  server.on("/api/status", []() { server.send(200, "application/json", statusJson()); });
  server.on("/api/history", handleHistory);
  server.on("/api/pay", handlePay);
  server.on("/api/arm", handleArm);
  server.on("/api/cancel", handleCancel);
  server.on("/api/code", handleCode);
  server.on("/api/tickets", handleTickets);
  server.on("/api/exitarm", handleExitArm);
  server.on("/api/admin", handleAdmin);
  server.on("/api/alerts", handleAlerts);
}
