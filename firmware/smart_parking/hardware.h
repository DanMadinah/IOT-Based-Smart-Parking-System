#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

// IR sensors
bool rawEntry();
bool rawExit();
void readIR();
bool entrySeen();
bool exitSeen();

// LCD, LEDs, buzzer
String pad16(String s);
void showScreen(String status);
void screen(String status);
void updateLeds();
void startBeeps(int n);
void updateBuzzer();

// Servo gate & non-blocking wait
bool moveGate(int target);
void closeGateSure();
void waitMs(unsigned long ms);

// Ultrasonic wall sensors & sensor health
float readCm(int echoPin);
bool isClose(float d);
void checkWallSensors();
void checkHealth();

#endif // HARDWARE_H
