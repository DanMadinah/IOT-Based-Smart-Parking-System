#ifndef FIREBASE_SYNC_H
#define FIREBASE_SYNC_H

#include <Arduino.h>

// Periodically pushes live lot state to Firebase Realtime Database
// and executes remote commands sent from the public Vercel dashboard.
void syncFirebase();

#endif // FIREBASE_SYNC_H
