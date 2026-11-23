#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <time.h>

// Load user's private config.h if present; otherwise fall back to config.example.h
#if __has_include("config.h")
  #include "config.h"
#else
  #include "config.example.h"
#endif

// Defaults if user's existing config.h doesn't define Firebase settings yet
#ifndef FIREBASE_DB_URL
  #define FIREBASE_DB_URL ""
#endif
#ifndef FIREBASE_AUTH
  #define FIREBASE_AUTH ""
#endif

// ---------- Types ----------
struct Event {
  time_t epoch;
  unsigned long ms;
  String text;
};

enum RS {
  R_NONE,
  R_RES,
  R_ARMED,
  R_ENTER,
  R_USED,
  R_EXP,
  R_CANCEL,
  R_PARKED,
  R_EXARM,
  R_LEFT
};

enum State { IDLE, ENTERING, EXITING };

struct Res {
  RS st = R_NONE;
  String token = "";
  String name = "";
  int no = 0;
  unsigned long created = 0;
  unsigned long armedAt = 0;
  unsigned long exArmAt = 0;
  int wrong = 0;
  int totalWrong = 0;
  int lockLevel = 0;
  unsigned long lockedUntil = 0;
  uint32_t ip = 0;
};

// ---------- Buffer Sizes ----------
constexpr int HN     = 90;
constexpr int MAXRES = 12;
constexpr int MAXEV  = 40;
constexpr int MAXAL  = 10;

// ---------- Hardware Objects ----------
extern LiquidCrystal_I2C* lcd;
extern Servo gate;
extern WebServer server;
extern Preferences prefs;

// ---------- Runtime Settings ----------
extern int maxCars;
extern bool secureMode;
extern bool muted;

// ---------- Gate & Lot State ----------
extern State state;
extern int carCount;
extern bool seenSecond;
extern bool gateIsOpen;
extern bool manualMode;
extern bool apMode;
extern int pendingGate;
extern int simCmd;
extern int demoCmd;
extern unsigned long stateStart;
extern unsigned long clearSince;
extern String lastStatus;
extern int lastCount;
extern int lastFree;

// ---------- Wall Proximity Sensors ----------
extern int warnHits;
extern bool warnActive;
extern unsigned long lastWarnCheck;
extern unsigned long lastWarnEvent;
extern float lastD1;
extern float lastD2;

// ---------- IR Debounce Filter ----------
extern unsigned long ignoreUntil;
extern unsigned long eLowSince, xLowSince, eHighSince, xHighSince;
extern bool eStable, xStable;

// ---------- Stats, Health & Tailgating ----------
extern int entriesToday, exitsToday, peakCars;
extern int paidTotal, refundTotal;
extern unsigned long entryOnSince, exitOnSince;
extern bool entryStuck, exitStuck;

extern int tailExtra;
extern bool eCleared;
extern bool tailLatched;
extern unsigned long tailUntil;
extern bool tailAlarm;
extern bool tailSeenExit;
extern bool tailExitFree;
extern unsigned long tailAlarmAt;
extern unsigned long tailClear;
extern unsigned long tailBeepT;

extern uint8_t hist[HN];
extern int hCount;
extern unsigned long lastHist;

// ---------- Reservations, Tickets & Security ----------
extern Res res[MAXRES];
extern int ticketNo;
extern int curRes;
extern int curExit;
extern bool exitFlagged;
extern unsigned long adminExitUntil;

extern String code;
extern int codeFor;
extern unsigned long codeAt;
extern bool codeOk;

extern unsigned long blinkUntil;
extern unsigned long gentleUntil;
extern unsigned long lcdMsgUntil;
extern bool entryFlagged;
extern unsigned long unconfStart;
extern bool unconfEsc;
extern int beepCount;
extern unsigned long beepT;

// ---------- Per-IP Cooldown ----------
extern uint32_t cdIp[4];
extern unsigned long cdUntil[4];
extern int cdNext;

// ---------- Event Log & Alerts ----------
extern Event events[MAXEV];
extern int evCount;
extern Event alerts[MAXAL];
extern int alCount;
extern int ac[6];
extern int wrongCodes;
extern unsigned long alertFlashUntil;
extern int badTicketless;
extern unsigned long badWin;

#endif // STATE_H
