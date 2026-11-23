# Firebase Realtime Database Setup (Live Hardware ↔ Vercel Sync)

Connect your physical ESP32 to your public Vercel website in ~3 minutes using Firebase's free Spark plan (**no extra Arduino libraries required**—the firmware uses the ESP32's built-in `HTTPClient`).

## Step 1: Create a Free Firebase Realtime Database
1. Go to [console.firebase.google.com](https://console.firebase.google.com) and click **Add project** (e.g., `smart-parking-live`).
2. In the left sidebar, click **Build → Realtime Database → Create Database**.
3. Pick a region near you (e.g., **Singapore `asia-southeast1`**).
4. Start in **Test mode** (you can tighten rules later) and click **Enable**.
5. Copy your Database URL from the top of the Data tab, e.g.:
   `https://smart-parking-live-default-rtdb.asia-southeast1.firebasedatabase.app`

## Step 2: Paste the URL in Two Places

### A) ESP32 Firmware (`firmware/smart_parking/config.h`)
Open your private `firmware/smart_parking/config.h` and add:
```cpp
#define FIREBASE_DB_URL "https://YOUR-PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH   ""
```
*(Make sure `WIFI_NAME` and `WIFI_PASS` are set to a 2.4 GHz Wi-Fi network with internet access.)*

### B) Public Website (`web/js/firebase-config.js`)
Open `web/js/firebase-config.js` and paste the same URL:
```js
window.FIREBASE_DB_URL = "https://YOUR-PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app";
```

## Step 3: Flash ESP32 & Push to GitHub / Vercel
1. Open `firmware/smart_parking/smart_parking.ino` in Arduino IDE and click **Upload**.
2. Push `web/js/firebase-config.js` to GitHub (`git add web/js/firebase-config.js && git commit -m "chore: set Firebase DB URL" && git push`).
3. Vercel auto-redeploys—your public website now displays **real-time sensor readings, cars, gate status, and controls** directly from your physical ESP32!
