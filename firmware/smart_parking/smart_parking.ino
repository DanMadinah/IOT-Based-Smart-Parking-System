// ============================================================================
// Smart Parking System — ESP32 Firmware Entry Point
// ============================================================================
#include <ESPmDNS.h>
#include <Wire.h>
#include "state.h"
#include "hardware.h"
#include "parking.h"
#include "demo.h"
#include "web_api.h"
#include "firebase_sync.h"

// ---------- Global Objects & State Definitions ----------
LiquidCrystal_I2C* lcd = nullptr;
Servo gate;
WebServer server(80);
Preferences prefs;

int maxCars = DEFAULT_MAX_CARS;
bool secureMode = false;
bool muted = false;

State state = IDLE;
int carCount = 0;
bool seenSecond = false;
bool gateIsOpen = false;
bool manualMode = false;
bool apMode = false;
int pendingGate = -1;
int simCmd = 0;
int demoCmd = 0;
unsigned long stateStart = 0;
unsigned long clearSince = 0;
String lastStatus = "";
int lastCount = -1;
int lastFree = -1;

int warnHits = 0;
bool warnActive = false;
unsigned long lastWarnCheck = 0;
unsigned long lastWarnEvent = 0;
float lastD1 = -1, lastD2 = -1;

unsigned long ignoreUntil = 0;
unsigned long eLowSince = 0, xLowSince = 0, eHighSince = 0, xHighSince = 0;
bool eStable = false, xStable = false;

int entriesToday = 0, exitsToday = 0, peakCars = 0;
int paidTotal = 0, refundTotal = 0;
unsigned long entryOnSince = 0, exitOnSince = 0;
bool entryStuck = false, exitStuck = false;

int tailExtra = 0;
bool eCleared = false;
bool tailLatched = false;
unsigned long tailUntil = 0;
bool tailAlarm = false;
bool tailSeenExit = false;
bool tailExitFree = false;
unsigned long tailAlarmAt = 0;
unsigned long tailClear = 0;
unsigned long tailBeepT = 0;

uint8_t hist[HN];
int hCount = 0;
unsigned long lastHist = 0;

Res res[MAXRES];
int ticketNo = 0;
int curRes = -1;
int curExit = -1;
bool exitFlagged = false;
unsigned long adminExitUntil = 0;

String code = "";
int codeFor = -1;
unsigned long codeAt = 0;
bool codeOk = false;

unsigned long blinkUntil = 0;
unsigned long gentleUntil = 0;
unsigned long lcdMsgUntil = 0;
bool entryFlagged = false;
unsigned long unconfStart = 0;
bool unconfEsc = false;
int beepCount = 0;
unsigned long beepT = 0;

uint32_t cdIp[4] = {0, 0, 0, 0};
unsigned long cdUntil[4] = {0, 0, 0, 0};
int cdNext = 0;

Event events[MAXEV];
int evCount = 0;
Event alerts[MAXAL];
int alCount = 0;
int ac[6] = {0, 0, 0, 0, 0, 0};
int wrongCodes = 0;
unsigned long alertFlashUntil = 0;
int badTicketless = 0;
unsigned long badWin = 0;

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(IR_ENTRY, INPUT);
  pinMode(IR_EXIT, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(ECHO2, INPUT);

  prefs.begin("lot", false);
  maxCars      = prefs.getInt("max", maxCars);
  secureMode   = prefs.getBool("sec", secureMode);
  muted        = prefs.getBool("mute", false);
  carCount     = prefs.getInt("cars", 0);
  entriesToday = prefs.getInt("in", 0);
  exitsToday   = prefs.getInt("out", 0);
  peakCars     = prefs.getInt("peak", 0);
  paidTotal    = prefs.getInt("paid", 0);
  refundTotal  = prefs.getInt("ref", 0);

  gate.setPeriodHertz(50);
  gate.attach(SERVO_PIN, 500, 2400);
  gate.write(GATE_CLOSED);
  delay(600);
  gate.detach();

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd = new LiquidCrystal_I2C(LCD_ADDR, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->setCursor(0, 0);
  lcd->print("Smart Parking");
  lcd->setCursor(0, 1);
  lcd->print("Connecting WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
    MDNS.begin(MDNS_HOST);
    Serial.print("Open on your phone: http://");
    Serial.println(WiFi.localIP());
    Serial.print("or http://");
    Serial.print(MDNS_HOST);
    Serial.println(".local");
    Serial.println("Admin page: /admin  (user: admin)");
    lcd->setCursor(0, 1);
    lcd->print(pad16(WiFi.localIP().toString()));
    delay(3000);
  } else {
    apMode = true;
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_NAME, AP_PASS);
    Serial.println("No home Wi-Fi. Hotspot started.");
    Serial.print("Join Wi-Fi '");
    Serial.print(AP_NAME);
    Serial.print("' (password ");
    Serial.print(AP_PASS);
    Serial.println(") then open http://192.168.4.1");
    lcd->setCursor(0, 0);
    lcd->print(pad16("Hotspot mode"));
    lcd->setCursor(0, 1);
    lcd->print(pad16("192.168.4.1"));
    delay(3500);
  }

  registerRoutes();
  server.begin();

  addEvent("System started");
  updateLeds();
}

// ---------- Main Loop ----------
void loop() {
  readIR();
  server.handleClient();
  updateReservations();
  updateExitTickets();

  if (pendingGate >= 0) {
    int t = pendingGate;
    pendingGate = -1;
    moveGate(t);
  }
  if (simCmd != 0) {
    int c = simCmd;
    simCmd = 0;
    runSim(c);
  }
  if (demoCmd != 0) {
    demoCmd = 0;
    runDemo();
  }

  bool e = entrySeen();
  bool x = exitSeen();
  if (!x) exitFlagged = false;

  if (!e) {
    entryFlagged = false;
    unconfEsc = false;
    gentleUntil = 0;
    if (state == IDLE) clearCode();
  }

  if (manualMode) {
    screen("MANUAL MODE");
  } else if (state == IDLE) {
    if (e) {
      if (carCount >= maxCars) {
        screen("FULL-NO ENTRY");
        if (!entryFlagged) {
          entryFlagged = true;
          startBeeps(1);
          addEvent("Lot full: entry denied");
        }
      } else {
        int a = armedIndex();
        if (a >= 0) {
          if (!secureMode) {
            beginEntry(a);
          } else {
            if (millis() < res[a].lockedUntil) {
              screen("LOCKED");
            } else {
              if (codeFor != a) makeCode(a);
              if (codeOk) beginEntry(a);
              else screen("Code: " + code);
            }
          }
        } else if (reservedCount() > 0) {
          screen("Tap I'M AT GATE");
          if (!entryFlagged) {
            entryFlagged = true;
            unconfStart = millis();
            unconfEsc = false;
            startBeeps(1);
            gentleUntil = millis() + UNCONF_MS;
            addEvent("Car at gate: driver has not tapped yet");
          }
          if (!unconfEsc && millis() - unconfStart > UNCONF_MS) {
            unconfEsc = true;
            gentleUntil = 0;
            blinkUntil = millis() + 4000;
            startBeeps(2);
            addAlert("Car at gate not confirmed: paid driver did not tap", 1);
          }
        } else {
          screen("PAY FIRST");
          if (!entryFlagged) {
            entryFlagged = true;
            blinkUntil = millis() + 4000;
            startBeeps(2);
            addAlert("Unpaid car at gate", 0);
          }
        }
      }
    } else if (x) {
      int xi = exitArmedIndex();
      if (!EXIT_STRICT || xi >= 0 || millis() < adminExitUntil) {
        curExit = xi;
        state = EXITING;
        seenSecond = false;
        clearSince = 0;
        stateStart = millis();
        screen("Goodbye");
        addEvent(xi >= 0 ? "Car leaving: " + res[xi].name : String("Car at exit: gate opening"));
        moveGate(GATE_OPEN);
      } else {
        screen("TAP I'M EXITING");
        if (!exitFlagged) {
          exitFlagged = true;
          blinkUntil = millis() + 4000;
          startBeeps(2);
          addAlert("Exit refused: no approved exit ticket", -1);
        }
      }
    } else {
      screen(freeSpaces() <= 0 ? "LOT FULL" : "Welcome");
    }
  } else {  // ENTERING or EXITING
    bool second = (state == ENTERING) ? x : e;
    if (second) seenSecond = true;

    if (state == ENTERING) {
      if (seenSecond && !e) eCleared = true;
      if (!e) tailLatched = false;
      if (!tailAlarm && seenSecond && eCleared && e && !tailLatched &&
          entryOnSince > 0 && millis() - entryOnSince > 400) {
        tailLatched = true;
        tailAlarm = true;
        tailSeenExit = false;
        tailExitFree = !x;
        tailClear = 0;
        tailAlarmAt = millis();
        addAlert("Possible tailgating: another car followed. Gate held open", 5);
      }
      if (tailAlarm) {
        if (!x) tailExitFree = true;
        if (x && tailExitFree) tailSeenExit = true;
        if (millis() - tailBeepT > 700) { tailBeepT = millis(); startBeeps(1); }
        blinkUntil = millis() + 500;
        tailUntil = millis() + 1500;
        alertFlashUntil = millis() + 1500;
        showScreen("TAILGATING!");
        if (tailSeenExit && !e && !x) {
          if (tailClear == 0) tailClear = millis();
          if (millis() - tailClear >= CLEAR_DELAY) {
            tailAlarm = false;
            tailExtra++;
            blinkUntil = 0;
            addEvent("Tailgating car has passed. Alarm ended");
          }
        } else {
          tailClear = 0;
        }
        if (tailAlarm && millis() - tailAlarmAt > TAIL_TIMEOUT) {
          tailAlarm = false;
          blinkUntil = 0;
          addEvent("Tailgating alarm timed out. Check the gate");
        }
      }
    }

    if (!tailAlarm && seenSecond && !e && !x) {
      if (clearSince == 0) clearSince = millis();
      if (millis() - clearSince >= CLEAR_DELAY) {
        if (moveGate(GATE_CLOSED)) {
          if (state == ENTERING) {
            int n = 1 + tailExtra;
            carCount += n;
            entriesToday += n;
            tailExtra = 0;
            if (carCount > peakCars) peakCars = carCount;
            String tn = "";
            if (curRes >= 0) {
              tn = " (" + ticketName(res[curRes].no) + ")";
              res[curRes].st = R_PARKED;
              curRes = -1;
            }
            addEvent("Car entered" + tn + ". Cars: " + String(carCount));
          } else if (carCount > 0) {
            carCount--;
            exitsToday++;
            String who = "";
            if (curExit >= 0) { who = " (" + res[curExit].name + ")"; res[curExit].st = R_LEFT; curExit = -1; }
            adminExitUntil = 0;
            addEvent("Car left" + who + ". Cars: " + String(carCount));
          }
          saveStats();
          updateLeds();
          screen(freeSpaces() <= 0 ? "LOT FULL" : "Thank you");
          state = IDLE;
        } else {
          clearSince = 0;
        }
      }
    } else {
      clearSince = 0;
    }

    if (!seenSecond && millis() - stateStart > PASS_TIMEOUT) {
      if (moveGate(GATE_CLOSED)) {
        if (state == ENTERING && curRes >= 0) {
          res[curRes].st = R_RES;
          res[curRes].created = millis();
          curRes = -1;
        }
        if (curExit >= 0) { res[curExit].st = R_PARKED; curExit = -1; }
        state = IDLE;
        addEvent("No car came through: cancelled");
      } else {
        stateStart = millis();
      }
    }
  }

  updateLeds();
  checkWallSensors();
  updateBuzzer();
  checkHealth();
  sampleHistory();

  // Auto-reconnect only if we started in Wi-Fi STA mode (keep Hotspot mode rock-solid)
  static unsigned long lastWifiRetry = 0;
  if (!apMode && WiFi.status() != WL_CONNECTED && millis() - lastWifiRetry >= 15000) {
    lastWifiRetry = millis();
    Serial.println("Wi-Fi dropped. Reconnecting...");
    WiFi.reconnect();
  }

  syncFirebase();
}
