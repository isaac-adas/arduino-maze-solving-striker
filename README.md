<p align="center">

# STRIKER v3

[![Arduino UNO](https://img.shields.io/badge/Arduino-UNO-00979D?style=for-the-badge&logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://www.arduino.cc/reference/en/)
[![MIT License](https://img.shields.io/badge/License-MIT-f7df1e?style=for-the-badge)](LICENSE)

[![Solve the maze](https://img.shields.io/badge/Job-Solve%20the%20maze-e63946?style=for-the-badge)](#getting-started)
[![Follow the wall](https://img.shields.io/badge/Strategy-Follow%20the%20wall-6a4c93?style=for-the-badge)](#control-panel)
[![3x HC-SR04](https://img.shields.io/badge/Eyes-3x%20HC--SR04-f4a261?style=for-the-badge)](#wiring)
[![L298N](https://img.shields.io/badge/Driver-L298N-264653?style=for-the-badge)](#wiring)

**Wall-following maze robot for Arduino Uno.**

Three ultrasonic sensors, an L298N, and a tab-and-slot plate. No camera. Sensor covers push into slots in the body. Zip ties hold the battery and the driver. No screws and no glue.

[photo here of the finished robot]

[![STRIKER maze run](https://img.youtube.com/vi/cDdMQnneDAg/hqdefault.jpg)](https://youtu.be/cDdMQnneDAg)

Build notes: [Instructables article URL here]

<br>

[Sketch](#the-sketch) · [Try the wires](#try-the-wires-first) · [Parts](#parts) · [Wiring](#wiring) · [Start](#getting-started) · [Control panel](#control-panel) · [Chassis](#chassis) · [Repo](#repo)

</p>

---

## Repo

```
arduino-maze-solving-striker/
├── Striker_v3/Striker_v3.ino          maze sketch
├── SensorCheck/SensorCheck.ino        distances only, motors off
├── MotorDirection/MotorDirection.ino  one wheel, then the other
├── design/
│   ├── Striker_v3_Chassis.dxf         laser file, 250 x 300 mm
│   └── Striker_v3_Chassis.pdf
├── docs/PIN_LAYOUT.md
├── libraries/NewPing/                 ultrasonic library
├── LICENSE
└── README.md
```

---

## The sketch

One file: [`Striker_v3/Striker_v3.ino`](Striker_v3/Striker_v3.ino).

Set `navigation` in the control panel:

| `navigation` | Steering |
|---|---|
| `SIMPLE_STEER` | Corrects in proportion to the distance error |
| `PID_STEER` | Proportional, integral, and derivative correction |

The sketch ships on `PID_STEER`. Switch to `SIMPLE_STEER` if you want one correction from `steerStrength` instead. Same wires either way.

Every setting you should change is in the **control panel** at the top of the sketch.

---

## Try the wires first

Upload one of these before the maze sketch. Same pins. Wheels off the ground for the motor test.

| Sketch | What you should see |
|---|---|
| [`SensorCheck`](SensorCheck/SensorCheck.ino) | Left, front, and right distances on the Serial Monitor at 115200. Motors stay off. |
| [`MotorDirection`](MotorDirection/MotorDirection.ino) | Left wheel, then right wheel, then both, each for one second. If a wheel goes backward, swap that motor's two wires. Upload nothing after the swap. |

---

## Parts

| Piece | Qty | Notes |
|---|---:|---|
| Arduino UNO | 1 | The brain |
| L298N motor driver | 1 | Remove the ENA and ENB jumpers |
| HC-SR04 ultrasonic sensor | 3 | Left, front, right |
| DC motors + wheels | 2 | |
| 12V battery pack | 1 | Powers the motors through the L298N |
| LEDs + 220–330 Ω resistors | 3 | Optional status lights |
| Chassis, jumpers, zip ties | | See [design/](design/) |

---

## Wiring

The sensors use the analog pins as digital pins, so the PWM pins stay free for the motors.

[photo here of the sensors on the front edge]

```
                 FRONT
        [ L ]    [ F ]    [ R ]
 TRIG     A3       A4       A5
 ECHO     A0       A1       A2
```

| Sensor | TRIG | ECHO |
|---|---|---|
| Left | A3 | A0 |
| Front | A4 | A1 |
| Right | A5 | A2 |

All three sensors: **VCC → 5V**, **GND → GND**.

| L298N | Arduino | Job |
|---|---|---|
| IN1 | D4 | Right motor direction |
| IN2 | D5 | Right motor direction |
| IN3 | D2 | Left motor direction |
| IN4 | D3 | Left motor direction |
| ENA | D11 | Right speed (PWM) |
| ENB | D10 | Left speed (PWM) |

| LED | Pin |
|---|---|
| Status | D13 |
| Following left | D8 |
| Following right | D9 |

> **Power.** Motor current comes from the battery through the L298N, never from an Arduino pin. Tie the Arduino, L298N, sensor, and battery grounds together.

Full pin map: [`docs/PIN_LAYOUT.md`](docs/PIN_LAYOUT.md).

---

## Getting started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install **NewPing**. A copy is in [`libraries/NewPing`](libraries/NewPing), or use the IDE Library Manager.
3. Open `Striker_v3`.
4. In the control panel, set `strategy` to `FOLLOW_AUTO`, `FOLLOW_LEFT`, or `FOLLOW_RIGHT`.
5. Leave `runCalibration` set to `false`.
6. Upload, open the Serial Monitor at **115200**, and set the robot in the maze.

Start slow. Raise the speed once it tracks the wall cleanly.

---

## Control panel

Change these at the top of the sketch. The rest of the file is the program.

### Which wall

| Set `strategy` to | What the robot does |
|---|---|
| `FOLLOW_AUTO` | Picks a wall at the first front wall |
| `FOLLOW_LEFT` | Always follows the left wall |
| `FOLLOW_RIGHT` | Always follows the right wall |

`autoDefault` is only for `FOLLOW_AUTO`, when both sides are open or both are blocked.

### Passage width

`corridorWidthCm` is one corridor, measured from the left wall to the right wall.

A **30 cm** passage keeps the original distances:

| | Side wall | Front stop | Follow gap |
|---|---:|---:|---:|
| `SIMPLE_STEER` | 13 cm | 8 cm | 10 cm |
| `PID_STEER` | 13 cm | 7 cm | 10 cm |

Change `corridorWidthCm` and those three distances scale with it.

### Try other distances

1. Set `manualDistances` to `true`.
2. Edit `manualSideWallCm`, `manualFrontStopCm`, and `manualFollowCm`.

### Calibrate

Set `runCalibration` to `true`. The motors stay off.

Open the Serial Monitor at **115200**, put the robot in a passage facing along it, and it will:

1. Collect readings for `calibrationSeconds` (5 by default).
2. Print min, average, and max for each sensor.
3. Print the air gap between the side sensors, and say if that is wider than `corridorWidthCm`.
4. Print three numbers you can copy into the manual distance settings.
5. Keep printing live readings, marked `WALL` or `open`, so you can move the robot and see what the current distances do.

Set `runCalibration` back to `false` before a real run.

### Also in the panel

| Setting | What it changes |
|---|---|
| Speed | How fast it drives and turns |
| `navigation` | `SIMPLE_STEER` or `PID_STEER` |
| `steerStrength` | `SIMPLE_STEER`. How hard it corrects |
| `PID_KP`, `PID_KI`, `PID_KD` | `PID_STEER`. How hard it corrects |

Once the walls end, the robot stops so it does not drive off the stage.

---

## Chassis

The laser-cut plate is **250 × 300 mm**, **3 mm** acrylic. Files and build notes are in [`design/`](design/).

A ready-made 2WD chassis works too, as long as the Arduino, driver, and three sensors fit.

[photo here of the plate from above]

[photo here of the tab and slot cover]

[photo here of stack]

---

<p align="center">

MIT. Free to use, modify, and share. See [LICENSE](LICENSE).

</p>
