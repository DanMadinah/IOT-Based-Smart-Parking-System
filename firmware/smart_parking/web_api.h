#ifndef WEB_API_H
#define WEB_API_H

#include <Arduino.h>

void registerRoutes();

// Shared JSON builders & command dispatcher (used by both local WebServer & Firebase sync)
String statusJson();
String historyJson();
String alertsJson();
String allTicketsJson();
String executeRemoteCommand(const String& action, const String& arg1, const String& arg2);

#endif // WEB_API_H
