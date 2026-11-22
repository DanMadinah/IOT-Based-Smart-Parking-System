#ifndef PARKING_H
#define PARKING_H

#include <Arduino.h>
#include "state.h"

// Cooldowns
void setCooldown(uint32_t ip);
long cooldownLeft(uint32_t ip);

// Events, alerts, stats & occupancy history
void addEvent(String text);
String stamp(const Event& e);
void addAlert(String text, int kind);
void saveStats();
void sampleHistory();

// Reservations & tickets
String ticketName(int no);
bool isActive(RS s);
bool slotFree(RS s);
int reservedCount();
int freeSpaces();
int findToken(const String& t);
int armedIndex();
int exitArmedIndex();
bool atGateNow();
void updateReservations();
void updateExitTickets();

// LCD security code & entry flow
void clearCode();
void makeCode(int a);
unsigned long lockMs(int level);
void beginEntry(int a);

#endif // PARKING_H
