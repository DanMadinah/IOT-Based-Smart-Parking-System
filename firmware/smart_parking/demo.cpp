#include "demo.h"
#include "state.h"
#include "hardware.h"
#include "parking.h"

void runSim(int c) {
  if (c == 1) {
    if (freeSpaces() <= 0) {
      addEvent("Demo: lot full, entry denied");
      startBeeps(1);
      return;
    }
    addEvent("Demo: paid car entering");
    moveGate(GATE_OPEN);
    waitMs(1000);
    carCount++;
    entriesToday++;
    if (carCount > peakCars) peakCars = carCount;
    saveStats();
    updateLeds();
    showScreen(freeSpaces() <= 0 ? "LOT FULL" : "Thank you");
    addEvent("Demo: car entered. Cars: " + String(carCount));
    closeGateSure();
  } else if (c == 2) {
    blinkUntil = millis() + 4000;
    startBeeps(2);
    addEvent("Demo: unpaid car at gate");
  } else if (c == 3) {
    if (carCount <= 0) {
      addEvent("Demo: lot is empty");
      return;
    }
    addEvent("Demo: car leaving");
    moveGate(GATE_OPEN);
    waitMs(1000);
    carCount--;
    exitsToday++;
    saveStats();
    updateLeds();
    showScreen("Goodbye");
    addEvent("Demo: car left. Cars: " + String(carCount));
    closeGateSure();
  }
}

void runDemo() {
  int sCars = carCount, sIn = entriesToday, sOut = exitsToday, sPeak = peakCars;
  int sPaid = paidTotal, sRef = refundTotal;
  addEvent("DEMO started");
  waitMs(1500);

  int slot = -1;
  for (int i = 0; i < MAXRES; i++) if (slotFree(res[i].st)) { slot = i; break; }
  if (slot >= 0) {
    ticketNo++;
    res[slot].st = R_RES;
    res[slot].no = ticketNo;
    res[slot].name = "Demo";
    res[slot].created = millis();
    res[slot].token = "demo" + String(ticketNo);
    res[slot].wrong = 0; res[slot].totalWrong = 0;
    res[slot].lockLevel = 0; res[slot].lockedUntil = 0; res[slot].ip = 0;
    paidTotal++;
    addEvent("Demo: payment RM2, space reserved: " + ticketName(ticketNo));
    waitMs(3000);
    res[slot].st = R_ARMED;
    res[slot].armedAt = millis();
    addEvent("Demo: driver tapped I'm at the gate");
    waitMs(2500);
    res[slot].st = R_ENTER;
    addEvent("Demo: paid car entering, gate opens");
    moveGate(GATE_OPEN);
    waitMs(1500);
    carCount++;
    entriesToday++;
    res[slot].st = R_USED;
    updateLeds();
    showScreen(freeSpaces() <= 0 ? "LOT FULL" : "Thank you");
    addEvent("Demo: car entered. Cars: " + String(carCount));
    closeGateSure();
    waitMs(2500);
  }

  carCount = maxCars;
  updateLeds();
  showScreen("LOT FULL");
  addEvent("Demo: lot is full");
  waitMs(3000);
  startBeeps(1);
  showScreen("FULL-NO ENTRY");
  addEvent("Demo: car refused, lot full");
  waitMs(3000);

  addEvent("Demo: car leaving");
  moveGate(GATE_OPEN);
  waitMs(1200);
  carCount--;
  exitsToday++;
  updateLeds();
  showScreen("Goodbye");
  addEvent("Demo: car left. Cars: " + String(carCount));
  closeGateSure();
  waitMs(2500);

  blinkUntil = millis() + 4000;
  startBeeps(2);
  showScreen("PAY FIRST");
  addAlert("Demo: unpaid car at gate", -1);
  waitMs(4500);

  showScreen("Tap I'M AT GATE");
  startBeeps(1);
  gentleUntil = millis() + 3500;
  addEvent("Demo: paid car at gate, driver has not tapped");
  waitMs(4000);
  gentleUntil = 0;
  blinkUntil = millis() + 3500;
  startBeeps(2);
  addAlert("Demo: car at gate not confirmed", -1);
  waitMs(4500);

  addEvent("Demo: wrong code, 2 tries left");
  waitMs(1500);
  addEvent("Demo: wrong code, 1 try left");
  waitMs(1500);
  blinkUntil = millis() + 3000;
  startBeeps(3);
  showScreen("LOCKED");
  addAlert("Demo: ticket locked 30 s: 3 wrong codes", -1);
  waitMs(4000);
  addAlert("Demo: ticket blocked after 6 wrong codes (demo refund)", -1);
  startBeeps(3);
  waitMs(3500);

  carCount = sCars; entriesToday = sIn; exitsToday = sOut; peakCars = sPeak;
  paidTotal = sPaid; refundTotal = sRef;
  saveStats();
  updateLeds();
  lastCount = -1;
  showScreen("Welcome");
  addEvent("DEMO finished: counts restored");
}
