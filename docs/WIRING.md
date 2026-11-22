# Hardware & Wiring Guide

## Components Used

| Component | Qty | Purpose |
| --- | --- | --- |
| ESP32 Dev Module | 1 | Main microcontroller & Wi-Fi web server |
| IR Obstacle Sensor | 2 | Entry & exit vehicle detection |
| Ultrasonic Sensor (HC-SR04) | 2 | Wall-proximity warning inside lot |
| Servo Motor (SG90 / MG90S) | 1 | Automated barrier gate arm |
| 16x2 I2C LCD (`0x27`) | 1 | Live lot status, prompts & 4-digit security code |
| Active Buzzer (active-LOW) | 1 | Alerts, wall warning & confirmation beeps |
| Green & Red LEDs (+ 220Ω resistors) | 2 | Lot availability & alert status indicators |

## ESP32 GPIO Pinout

| Component | Pin | ESP32 GPIO | Notes |
| --- | --- | --- | --- |
| Entry IR Sensor | OUT | `GPIO 19` | Active-LOW (`LOW` = car detected) |
| Exit IR Sensor | OUT | `GPIO 16` | Active-LOW (`LOW` = car detected) |
| Ultrasonic 1 & 2 | TRIG | `GPIO 2` | Shared trigger pin |
| Ultrasonic 1 | ECHO | `GPIO 17` | Use a voltage divider if powered at 5V |
| Ultrasonic 2 | ECHO | `GPIO 15` | Use a voltage divider if powered at 5V |
| Servo Motor | SIG | `GPIO 18` | 50 Hz PWM (`500–2400 µs`); power from stable 5V rail |
| Active Buzzer | I/O | `GPIO 23` | Active-LOW (`LOW` = ON, `HIGH` = OFF) |
| Green LED | Anode | `GPIO 5` | Through 220Ω resistor to GND |
| Red LED | Anode | `GPIO 4` | Through 220Ω resistor to GND |
| 16x2 I2C LCD | SDA | `GPIO 21` | Default I2C SDA |
| 16x2 I2C LCD | SCL | `GPIO 22` | Default I2C SCL (address `0x27`) |

## Wiring Tips

- **Common Ground**: Connect the GND of the ESP32, sensors, LCD, and external 5V servo supply together.
- **Servo Power**: Powering the servo from an external 5V supply (rather than the ESP32 3.3V pin) prevents brownout resets when the barrier moves.
- **Safe-Close Tuning**: If the barrier arm triggers the IR sensor while closing, set `SAFE_CLOSE = false` in `firmware/smart_parking/config.h`.
