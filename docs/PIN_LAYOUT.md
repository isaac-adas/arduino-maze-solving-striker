# STRIKER v3 — Arduino UNO Pin Layout

Board: **Arduino UNO R3 / REV3 (ATmega328P)**.

## Reference material scanned (in `new-stuff/`)
1. `A000066-pinout.png` — official Arduino UNO REV3 pinout (SKU A000066, 6 Oct 2022 revision). **Primary source of truth.**
2. `Arduino-UNO-Parts-Marking.jpg` — components101-style labelled board photo.
3. `Arduino Uno Pinout, Specifications, Pin Configuration & Programming.url` -> https://components101.com/microcontrollers/arduino-uno (specs table).
4. `arduino-uno.pdf` — image-only PDF (same diagram rendered as JPEG, no extractable text).

Source of truth for the robot's pin assignments: `Striker_v3/Striker_v3.ino`.
The same map was already used by the earlier v2 sketch.

## Discrepancies found between references (resolved)
- **PWM pins:** official diagram + Arduino docs = **3, 5, 6, 9, 10, 11**. The
  components101 *specs table* text lists "3, 5, 6, 9, 11" and drops **10** — that
  is an error on their page; their own board image shows ~D10 as PWM. Our design
  uses D10 and D11 for motor speed (ENB/ENA) and both are genuinely PWM. OK.
- **I2C labels:** official = A4/SDA (D18), A5/SCL (D19). components101 writes
  "A5 (SCA)" — typo for SCL. No I2C device is used here, so no conflict.
- **SPI overlap:** D10=SS, D11=MOSI, D12=MISO, D13=SCK. We use D10/D11 for motor
  PWM and D13 for the status LED. Fine because SPI is not used.

Hardware: Arduino UNO + L298N motor driver + 3x HC-SR04 ultrasonic sensors +
2 DC gear motors + 3 status LEDs.

---

## Full pin map

| Arduino Pin | Port / Alt | PWM? | Used By | Signal | Direction | Notes |
|-------------|-----------|------|---------|--------|-----------|-------|
| D0 / RX     | PD0       | no   | (Serial) | USB RX | —        | Keep free — used by USB serial upload/debug |
| D1 / TX     | PD1       | no   | (Serial) | USB TX | —        | Keep free — used by USB serial upload/debug |
| D2          | PD2       | no   | L298N   | IN3 — Left motor dir 1  | OUT | `LEFT_MOTOR_1` / `en1` |
| D3          | PD3       | ~PWM | L298N   | IN4 — Left motor dir 2  | OUT | `LEFT_MOTOR_2` / `en2` (used digital) |
| D4          | PD4       | no   | L298N   | IN1 — Right motor dir 1 | OUT | `RIGHT_MOTOR_1` / `en3` |
| D5          | PD5       | ~PWM | L298N   | IN2 — Right motor dir 2 | OUT | `RIGHT_MOTOR_2` / `en4` (used digital) |
| D6          | PD6       | ~PWM | —       | (free) | —        | Available |
| D7          | PD7       | no   | —       | (free) | —        | Available |
| D8          | PB0       | no   | LED     | Left-strategy indicator | OUT | `LEFT_LED` / `led1` |
| D9          | PB1       | ~PWM | LED     | Right-strategy indicator| OUT | `RIGHT_LED` / `led2` (PWM pin, used digital) |
| D10         | PB2       | ~PWM | L298N   | **ENB** — Left motor speed  | PWM OUT | `LEFT_SPEED` / `enA`(v2 var name) |
| D11         | PB3       | ~PWM | L298N   | **ENA** — Right motor speed | PWM OUT | `RIGHT_SPEED` / `enB`(v2 var name) |
| D12         | PB4       | no   | —       | (free) | —        | Available |
| D13         | PB5       | no   | LED     | Status LED (also on-board LED) | OUT | `STATUS_LED` / `LED` |
| A0 / D14    | PC0       | no   | HC-SR04 Left  | ECHO  | IN  | `LEFT_ECHO`  — analog pin used as digital |
| A1 / D15    | PC1       | no   | HC-SR04 Front | ECHO  | IN  | `FRONT_ECHO` — analog pin used as digital |
| A2 / D16    | PC2       | no   | HC-SR04 Right | ECHO  | IN  | `RIGHT_ECHO` — analog pin used as digital |
| A3 / D17    | PC3       | no   | HC-SR04 Left  | TRIG  | OUT | `LEFT_TRIG`  — analog pin used as digital |
| A4 / D18    | PC4       | no   | HC-SR04 Front | TRIG  | OUT | `FRONT_TRIG` — analog pin used as digital (also SDA) |
| A5 / D19    | PC5       | no   | HC-SR04 Right | TRIG  | OUT | `RIGHT_TRIG` — analog pin used as digital (also SCL) |
| 5V          | —         | —    | Power   | +5V logic | —  | HC-SR04 VCC + L298N 5V logic |
| GND         | —         | —    | Power   | Ground | —     | Common ground: Arduino + L298N + sensors |
| VIN         | —         | —    | Power   | (optional) | — | Battery in if not powering via L298N reg |

---

## Grouped by module

### HC-SR04 Ultrasonic Sensors (3x)
All trigger/echo lines grouped on the analog header (A0–A5) used as digital,
deliberately keeping the PWM-capable pins free for motor control.

| Sensor | TRIG | ECHO |
|--------|------|------|
| Left   | A3 (D17) | A0 (D14) |
| Front  | A4 (D18) | A1 (D15) |
| Right  | A5 (D19) | A2 (D16) |

- VCC -> 5V, GND -> GND (shared).
- `MAX_DISTANCE` in code = 100 cm.

### L298N Motor Driver

| L298N Pin | Arduino Pin | Function |
|-----------|-------------|----------|
| IN1       | D4  | Right motor direction 1 |
| IN2       | D5  | Right motor direction 2 |
| IN3       | D2  | Left motor direction 1  |
| IN4       | D3  | Left motor direction 2  |
| ENA       | D11 | Right motor speed (PWM)  |
| ENB       | D10 | Left motor speed (PWM)   |

- ENA/ENB must stay on true PWM pins (D3, D5, D6, D9, D10, D11 are PWM on UNO).
  D10 and D11 are used here — both PWM-capable. Good.
- Motor supply (VS/12V) and GND come from the battery pack, not the Arduino.
- Remove the ENA/ENB jumpers on the L298N board so PWM speed control works.

### Status LEDs (3x)

| LED | Arduino Pin | Meaning |
|-----|-------------|---------|
| Status | D13 | Alive/heartbeat (shares on-board LED) |
| Left strategy  | D8 | Lit when following left wall  |
| Right strategy | D9 | Lit when following right wall |

- Each LED via a current-limiting resistor (~220–330 ohm) to GND.

### Power
- 5V: HC-SR04 sensors + L298N logic supply.
- GND: single common ground tying Arduino, L298N, sensors, and battery negative.
- Motor power: dedicated battery to L298N VS terminal (do NOT power motors from the Arduino 5V).

---

## Notes / warnings
- **D0 (RX) and D1 (TX) are intentionally unused** — they carry USB serial for
  uploading code and Serial Monitor debugging at 115200 baud.
- Sensors use A0–A5 as plain digital I/O; on the UNO these work as digital
  pins D14–D19. A4/A5 double as I2C (SDA/SCL) but there is no I2C device here,
  so no conflict.
- Free pins available for expansion: **D6, D7, D12** (D6 is PWM-capable).
- `Striker_v2.ino` variable names map as: en1=D2, en2=D3, en3=D4, en4=D5,
  enA=D10, enB=D11 (note: in v2 the `enA` variable drives the ENB/left channel
  and `enB` drives ENA/right — pin numbers are what matter and they match).

---

## Board electrical specs (from references, for the code update later)

| Spec | Value |
|------|-------|
| Microcontroller | ATmega328P (8-bit AVR) |
| Operating voltage | 5V |
| Recommended input (VIN/barrel) | 7–12V |
| Input voltage limits | 6–20V |
| Digital I/O pins | 14 (D0–D13), 6 are PWM |
| PWM pins | 3, 5, 6, 9, 10, 11 |
| Analog input pins | 6 (A0–A5), also usable as digital D14–D19 |
| DC current per I/O pin | 40 mA max (20 mA recommended) |
| DC current on 3.3V pin | 50 mA |
| Flash | 32 KB (0.5 KB bootloader) |
| SRAM | 2 KB |
| EEPROM | 1 KB |
| Clock speed | 16 MHz |
| External interrupt pins | 2, 3 |
| I2C (TWI) | A4 = SDA, A5 = SCL |
| SPI | D10 = SS, D11 = MOSI, D12 = MISO, D13 = SCK |
| On-board LED | D13 |

### Implications for the STRIKER wiring
- **Do not power the motors from any Arduino pin.** 40 mA/pin max — motors draw
  amps. Motor power must come from the battery via the L298N VS terminal.
- LEDs need series resistors (~220–330 ohm) to stay under the 40 mA/pin limit.
- HC-SR04 ECHO outputs 5V, safe for the 5V UNO inputs (no level shifting needed).
- Pins 10 & 11 (motor PWM) are confirmed hardware PWM — analogWrite() works.

_Last verified: against Arduino UNO REV3 official pinout (A000066, 6 Oct 2022)
plus components101 specs. All four `new-stuff/` references scanned._
