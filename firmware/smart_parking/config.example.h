#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Smart Parking System — Configuration
// Copy this file to `config.h` and fill in your Wi-Fi and admin credentials.
// `config.h` is listed in .gitignore so your secrets are never pushed to GitHub.
// ============================================================================

// ---------- Wi-Fi (2.4 GHz) ----------
constexpr const char* WIFI_NAME = "YOUR_WIFI_NAME";
constexpr const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ---------- Fallback Hotspot (used only if home Wi-Fi fails) ----------
constexpr const char* AP_NAME = "DanMadinah-Lot";
constexpr const char* AP_PASS = "parking123";
constexpr const char* MDNS_HOST = "danmadinah";

// ---------- Admin Login (username: admin) ----------
constexpr const char* ADMIN_PASS = "CHANGE_THIS_PASSWORD";

// ---------- Firebase Realtime Database (Cloud Sync for Vercel) ----------
// Leave empty ("") to run local-only. Example:
// #define FIREBASE_DB_URL "https://smart-parking-12345-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_DB_URL ""
#define FIREBASE_AUTH   ""  // optional legacy DB secret or auth token

// ---------- About Page Metadata ----------
constexpr const char* PROJECT_TITLE = "Smart Parking System";
constexpr const char* STUDENT_NAME  = "Muhammad Abdullahi Bukar";
constexpr const char* SUPERVISOR    = "Mohd Zaki Shahabuddin";

// ---------- Parking & Gate Tuning ----------
constexpr int   DEFAULT_MAX_CARS = 4;
constexpr float WARN_CM          = 6.0f;
constexpr int   GATE_CLOSED      = 0;
constexpr int   GATE_OPEN        = 90;
constexpr int   SERVO_STEP       = 2;       // smooth 50 Hz-matched stepping
constexpr int   SERVO_DELAY      = 18;      // ms per step (~20 ms servo period)
constexpr bool  SAFE_CLOSE       = true;    // false if the gate arm itself triggers the IR sensor
constexpr bool  EXIT_STRICT      = true;    // false = allow any car to exit without ticket

// ---------- Timers (milliseconds) ----------
constexpr unsigned long DEBOUNCE_MS  = 40;
constexpr unsigned long RELEASE_MS   = 400;
constexpr unsigned long CLEAR_DELAY  = 1200;
constexpr unsigned long PASS_TIMEOUT = 15000;
constexpr unsigned long RESERVE_MS   = 60000;  // 180000 when testing many cars
constexpr unsigned long ARM_MS       = 20000;
constexpr unsigned long CODE_MS      = 20000;
constexpr unsigned long UNCONF_MS    = 8000;
constexpr unsigned long COOLDOWN_MS  = 120000;
constexpr unsigned long STUCK_MS     = 60000;
constexpr unsigned long TAIL_TIMEOUT = 60000;

// ---------- ESP32 GPIO Pinout ----------
constexpr int IR_ENTRY  = 19;
constexpr int IR_EXIT   = 16;
constexpr int TRIG      = 2;
constexpr int ECHO1     = 17;
constexpr int ECHO2     = 15;
constexpr int BUZZER    = 23;
constexpr int LED_GREEN = 5;
constexpr int LED_RED   = 4;
constexpr int SERVO_PIN = 18;
constexpr int I2C_SDA   = 21;
constexpr int I2C_SCL   = 22;
constexpr int LCD_ADDR  = 0x27;

#endif // CONFIG_H
