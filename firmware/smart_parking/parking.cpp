#include "parking.h"
#include "hardware.h"

// ---------- Cooldown per phone ----------
void setCooldown(uint32_t ip) {
  if (ip == 0) return;
  for (int i = 0; i < 4; i++)
    if (cdIp[i] == ip) { cdUntil[i] = millis() + COOLDOWN_MS; return; }
  cdIp[cdNext] = ip;
  cdUntil[cdNext] = millis() + COOLDOWN_MS;
  cdNext = (cdNext + 1) % 4;
}

long cooldownLeft(uint32_t ip) {
  if (ip == 0) return 0;
  for (int i = 0; i < 4; i++)
    if (cdIp[i] == ip && millis() < cdUntil[i]) return (long)((cdUntil[i] - millis()) / 1000 + 1);
  return 0;
}

// ---------- Event Log & Alerts ----------
void addEvent(String text) {
  if (evCount == MAXEV) {
    for (int i = 0; i < MAXEV - 1; i++) events[i] = events[i + 1];
    evCount--;
  }
  time_t now = time(nullptr);
  events[evCount].epoch = (now > 100000) ? now : 0;
  events[evCount].ms = millis();
  events[evCount].text = text;
  evCount++;
}

String stamp(const Event& e) {
  if (e.epoch > 100000) {
    struct tm t;
    time_t ep = e.epoch;
    localtime_r(&ep, &t);
    char b[12];
    strftime(b, sizeof(b), "%H:%M:%S", &t);
    return String(b);
  }
  return "+" + String(e.ms / 1000) + "s";
}

void addAlert(String text, int kind) {
  addEvent(text);
  if (alCount == MAXAL) {
    for (int i = 0; i < MAXAL - 1; i++) alerts[i] = alerts[i + 1];
    alCount--;
  }
  time_t now = time(nullptr);
  alerts[alCount].epoch = (now > 100000) ? now : 0;
  alerts[alCount].ms = millis();
  alerts[alCount].text = text;
  alCount++;
  if (kind >= 0 && kind < 6) ac[kind]++;
  alertFlashUntil = millis() + 6000;
}

void saveStats() {
  prefs.putInt("cars", carCount);
  prefs.putInt("in", entriesToday);
  prefs.putInt("out", exitsToday);
  prefs.putInt("peak", peakCars);
  prefs.putInt("paid", paidTotal);
  prefs.putInt("ref", refundTotal);
}

void sampleHistory() {
  if (hCount > 0 && millis() - lastHist < 20000) return;
  lastHist = millis();
  if (hCount == HN) {
    memmove(hist, hist + 1, HN - 1);
    hCount--;
  }
  hist[hCount++] = (uint8_t)carCount;
}

// ---------- Reservation Helpers ----------
String ticketName(int no) {
  char b[10];
  sprintf(b, "P-%04d", no);
  return String(b);
}

bool isActive(RS s) { return s == R_RES || s == R_ARMED || s == R_ENTER; }
bool slotFree(RS s) { return s == R_NONE || s == R_EXP || s == R_CANCEL || s == R_USED || s == R_LEFT; }

int reservedCount() {
  int n = 0;
  for (int i = 0; i < MAXRES; i++) if (isActive(res[i].st)) n++;
  return n;
}

int freeSpaces() { return maxCars - carCount - reservedCount(); }

int findToken(const String& t) {
  if (t.length() == 0) return -1;
  for (int i = 0; i < MAXRES; i++) if (res[i].st != R_NONE && res[i].token == t) return i;
  return -1;
}

int armedIndex() {
  for (int i = 0; i < MAXRES; i++)
    if (res[i].st == R_ARMED && millis() - res[i].armedAt <= ARM_MS) return i;
  return -1;
}

int exitArmedIndex() {
  for (int i = 0; i < MAXRES; i++)
    if (res[i].st == R_EXARM && millis() - res[i].exArmAt <= ARM_MS) return i;
  return -1;
}

bool atGateNow() {
  return entrySeen() && state == IDLE && reservedCount() > 0 && armedIndex() < 0;
}

void updateReservations() {
  unsigned long now = millis();
  for (int i = 0; i < MAXRES; i++) {
    if (res[i].st == R_RES && now - res[i].created > RESERVE_MS) {
      res[i].st = R_EXP;
      refundTotal++;
      saveStats();
      addEvent("Reservation expired (demo refund): " + ticketName(res[i].no));
    } else if (res[i].st == R_ARMED && now - res[i].armedAt > ARM_MS) {
      res[i].st = R_RES;
      addEvent("Gate window ended: " + ticketName(res[i].no));
    }
  }
}

void updateExitTickets() {
  for (int i = 0; i < MAXRES; i++)
    if (res[i].st == R_EXARM && millis() - res[i].exArmAt > ARM_MS) {
      res[i].st = R_PARKED;
      addEvent("Exit window ended: " + ticketName(res[i].no));
    }
}

// ---------- LCD Security Code & Entry ----------
void clearCode() {
  code = "";
  codeFor = -1;
  codeOk = false;
}

void makeCode(int a) {
  code = String(random(1000, 10000));
  codeFor = a;
  codeAt = millis();
  codeOk = false;
  addEvent("LCD code shown for " + ticketName(res[a].no));
}

unsigned long lockMs(int level) {
  if (level <= 1) return 30000UL;
  if (level == 2) return 120000UL;
  return 300000UL;
}

void beginEntry(int a) {
  curRes = a;
  res[a].st = R_ENTER;
  clearCode();
  state = ENTERING;
  seenSecond = false;
  eCleared = false;
  tailLatched = false;
  tailAlarm = false;
  tailExtra = 0;
  clearSince = 0;
  stateStart = millis();
  gentleUntil = 0;
  showScreen("Please enter");
  addEvent("Paid car " + ticketName(res[a].no) + " (" + res[a].name + "): gate opening");
  moveGate(GATE_OPEN);
}
