# Security Design

## Goal
Allow a vehicle into (and out of) a private, paid car park only when a valid, paid ticket exists and the driver is physically at the gate—raising immediate, visible, and audible alerts whenever rules are violated.

## Five-Layer Defence-in-Depth Model

| Layer | What It Checks | What It Stops |
| :--- | :--- | :--- |
| **1. Network & Cloud Auth** | Local Wi-Fi / SoftAP access for on-site operation; ESP32-verified admin password for remote cloud commands | Unauthorized remote control |
| **2. Ticketed Payment (Demo)** | Flat RM2 demo payment creates a unique token and reserves a space for 60 s; unused/cancelled tickets auto-refund | Free-riders |
| **3. Arrival Confirmation** | Driver taps **"I'm at the gate"** within 20 s of reaching the entry sensor | Tickets used at the wrong time |
| **4. One-Time LCD Code (Secure Mode)** | Random 4-digit code shown only on the physical 16x2 LCD (valid 20 s, single-use) paired with the browser's ticket token | Remote entry by someone not physically at the gate |
| **5. Escalating Lockout** | 3 wrong codes lock the ticket (30 s → 2 min → 5 min); 6 wrong codes cancel the ticket (demo refund), raise an alert, and apply a 2-minute cooldown | Brute-force code guessing |

Exit is also protected: each parked vehicle has its own ticket and must be armed with **"I'm exiting"** (or approved by an Admin) before the barrier opens.

## Detection & Response Matrix

| Event | System Response |
| :--- | :--- |
| Unpaid car at entry gate | Gate stays closed, red LED blinks, 2 beeps, security alert logged |
| Paid driver did not tap "I'm at the gate" | Gentle reminder (1 beep, green LED blinks), escalates to alert after 8 s |
| Lot full | Entry refused, red LED on, new reservations blocked |
| Tailgating (second car follows) | Gate held open safely, buzzer pulses, red LED blinks, alert raised; clears when follower passes or admin resets |
| Exit without approved ticket | Gate stays closed, red LED blinks, 2 beeps, alert logged; admin can approve a 30 s exit window |
| Obstruction while gate closes | Servo immediately re-opens barrier (`SAFE_CLOSE`) |
| Vehicle too close to wall (`< 6 cm`) | Continuous buzzer warning via dual HC-SR04 ultrasonic sensors |

## Admin Protection
- **Local network (`/admin`)**: Protected by HTTP Basic Authentication (`admin` + `ADMIN_PASS` from `config.h`).
- **Cloud dashboard (Vercel)**: Protected by an Admin Login gate whose password is verified directly on the ESP32 against `ADMIN_PASS` (zero passwords stored in GitHub or frontend JavaScript).

## Prototype Scope & Production Hardening
This project is designed as a layered, defence-in-depth academic IoT prototype. Before a commercial deployment, the following upgrades are recommended:
- End-to-end TLS/HTTPS on the local controller and Firebase Authentication + tightened Realtime Database rules.
- Real payment gateway integration handled server-side (off the ESP32).
- Vehicle-level identity verification via ANPR (number-plate camera) or UHF RFID tags.
- Persistent off-device audit log storage and formal penetration testing.
