#include "hardware.h"
#include "state.h"
#include "parking.h"

// ---------- IR Filter ----------
bool rawEntry() { return digitalRead(IR_ENTRY) == LOW; }
bool rawExit()  { return digitalRead(IR_EXIT) == LOW; }

void readIR() {
  if (millis() < ignoreUntil) {
    eLowSince = 0; xLowSince = 0; eHighSince = 0; xHighSince = 0;
    eStable = false; xStable = false;
    return;
  }
  if (rawEntry()) {
    eHighSince = 0;
    if (eLowSince == 0) eLowSince = millis();
    if (millis() - eLowSince >= DEBOUNCE_MS) eStable = true;
  } else {
    eLowSince = 0;
    if (eStable) {
      if (eHighSince == 0) eHighSince = millis();
      if (millis() - eHighSince >= RELEASE_MS) { eStable = false; eHighSince = 0; }
    }
  }
  if (rawExit()) {
    xHighSince = 0;
    if (xLowSince == 0) xLowSince = millis();
    if (millis() - xLowSince >= DEBOUNCE_MS) xStable = true;
  } else {
    xLowSince = 0;
    if (xStable) {
      if (xHighSince == 0) xHighSince = millis();
      if (millis() - xHighSince >= RELEASE_MS) { xStable = false; xHighSince = 0; }
    }
  }
}

bool entrySeen() { return eStable; }
bool exitSeen()  { return xStable; }

// ---------- Screen, LEDs, Buzzer ----------
String pad16(String s) {
  while (s.length() < 16) s += ' ';
  return s.substring(0, 16);
}

void showScreen(String status) {
  int fr = freeSpaces();
  if (carCount == lastCount && fr == lastFree && status == lastStatus) return;
  lastCount = carCount;
  lastFree = fr;
  lastStatus = status;
  lcd->setCursor(0, 0);
  lcd->print(pad16("Cars:" + String(carCount) + "/" + String(maxCars) +
                   " Free:" + String(fr < 0 ? 0 : fr)));
  lcd->setCursor(0, 1);
  lcd->print(pad16(status));
}

void screen(String status) {
  if (millis() < lcdMsgUntil) return;
  showScreen(status);
}

void updateLeds() {
  if (millis() < blinkUntil) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, ((millis() / 250) % 2) ? HIGH : LOW);
    return;
  }
  if (millis() < gentleUntil) {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, ((millis() / 500) % 2) ? HIGH : LOW);
    return;
  }
  bool full = freeSpaces() <= 0;
  digitalWrite(LED_GREEN, full ? LOW : HIGH);
  digitalWrite(LED_RED, full ? HIGH : LOW);
}

void startBeeps(int n) {
  beepCount = n * 2;
  beepT = millis();
}

void updateBuzzer() {
  if (beepCount > 0 && millis() - beepT >= 150) {
    beepT = millis();
    beepCount--;
  }
  bool on = !muted && (warnActive || (beepCount > 0 && beepCount % 2 == 0));
  digitalWrite(BUZZER, on ? LOW : HIGH);
}

// ---------- Gate ----------
bool moveGate(int target) {
  static int pos = GATE_CLOSED;
  if (pos != target) {
    bool closing = (target < pos);
    ignoreUntil = millis() + 8000;
    gate.attach(SERVO_PIN, 500, 2400);
    int step = (target > pos) ? SERVO_STEP : -SERVO_STEP;
    unsigned long t0 = millis();
    int blocked = 0;
    while (pos != target) {
      pos += step;
      if ((step > 0 && pos > target) || (step < 0 && pos < target)) pos = target;
      gate.write(pos);
      delay(SERVO_DELAY);
      server.handleClient();
      updateLeds();
      updateBuzzer();
      if (SAFE_CLOSE && closing && millis() - t0 > 80) {
        if (rawEntry() || rawExit()) blocked++; else blocked = 0;
        if (blocked >= 5) {
          addEvent("Gate re-opened: something in the way");
          moveGate(GATE_OPEN);
          return false;
        }
      }
    }
    delay(220);
    gate.detach();
    ignoreUntil = millis() + 400;
  }
  gateIsOpen = (target == GATE_OPEN);
  return true;
}

void closeGateSure() {
  int tries = 0;
  while (!moveGate(GATE_CLOSED) && tries < 3) {
    tries++;
    waitMs(600);
  }
}

void waitMs(unsigned long ms) {
  unsigned long t = millis();
  while (millis() - t < ms) {
    server.handleClient();
    updateLeds();
    updateBuzzer();
    delay(5);
  }
}

// ---------- Wall Sensors ----------
float readCm(int echoPin) {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long t = pulseIn(echoPin, HIGH, 4000);
  if (t == 0) return -1;
  return t * 0.0343f / 2.0f;
}

bool isClose(float d) { return d >= 2 && d < WARN_CM; }

void checkWallSensors() {
  if (millis() - lastWarnCheck < 150) return;
  lastWarnCheck = millis();
  lastD1 = readCm(ECHO1);
  server.handleClient();
  delay(8);
  lastD2 = readCm(ECHO2);
  server.handleClient();
  bool was = warnActive;
  if (isClose(lastD1) || isClose(lastD2)) warnHits++;
  else warnHits = 0;
  warnActive = warnHits >= 2;
  if (warnActive && !was && millis() - lastWarnEvent > 5000) {
    addEvent("Wall warning: car too close");
    lastWarnEvent = millis();
  }
}

// ---------- Sensor Health ----------
void checkHealth() {
  bool e = entrySeen(), x = exitSeen();
  if (e) { if (entryOnSince == 0) entryOnSince = millis(); }
  else { entryOnSince = 0; entryStuck = false; }
  if (x) { if (exitOnSince == 0) exitOnSince = millis(); }
  else { exitOnSince = 0; exitStuck = false; }
  if (e && !entryStuck && millis() - entryOnSince > STUCK_MS) {
    entryStuck = true;
    addEvent("Health: entry sensor stuck?");
  }
  if (x && !exitStuck && millis() - exitOnSince > STUCK_MS) {
    exitStuck = true;
    addEvent("Health: exit sensor stuck?");
  }
}
