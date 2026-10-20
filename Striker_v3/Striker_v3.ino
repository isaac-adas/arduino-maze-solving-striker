/*
  STRIKER v3 — wall-following maze robot.
  Only edit a line that ends with <--   then upload.
*/

#include <NewPing.h>


// --- Control panel ---
// The word or number after = is your choice.

const int SIMPLE_STEER = 0;
const int PID_STEER    = 1;
// How the wheels correct while a wall is beside the robot.
//   SIMPLE_STEER  steerStrength times the distance error
//   PID_STEER     PID_KP, PID_KI, and PID_KD
const int navigation = PID_STEER ;   // <--

const int FOLLOW_AUTO  = 0;
const int FOLLOW_LEFT  = 1;
const int FOLLOW_RIGHT = 2;
// Which wall to follow.
//   FOLLOW_AUTO   picks at the first front wall
//   FOLLOW_LEFT   always left
//   FOLLOW_RIGHT  always right
const int strategy = FOLLOW_AUTO;      // <--

// Used only when strategy is FOLLOW_AUTO and both sides look the same.
//   FOLLOW_LEFT or FOLLOW_RIGHT
const int autoDefault = FOLLOW_LEFT;   // <--

// Width of one corridor, left wall to right wall, in cm.
const int corridorWidthCm = 30;        // <--

// Speed. 0 stopped, 255 full. Raise these only after it tracks the wall.
const int driveSpeed = 80;             // <-- straight
const int turnSpeed  = 70;             // <-- corners

// SIMPLE_STEER only. Higher turns harder. Lower if it wobbles.
const float steerStrength = 3.0;       // <--

// PID_STEER only. Each gain turns centimeters of error into PWM.
const float PID_KP    = 0.7;           // <-- current error. Higher reacts faster
const float PID_KI    = 0.4;           // <-- error that will not go away. Higher closes a steady gap
const float PID_KD    = 0.5;           // <-- how fast the error is changing. Higher resists a sudden swing
const float PID_LIMIT = 40;            // <-- largest PWM added to or removed from driveSpeed

// Leave the rest unless a sensor or the passage width is wrong.

// true = wheels stay off and the monitor shows distances. Open it at 115200.
const bool runCalibration     = false; // <--
const int  calibrationSeconds = 5;     // <-- how long to measure, in seconds

// false = distances come from corridorWidthCm.
// true  = type your own three distances below.
const bool manualDistances  = false;   // <--
const int manualSideWallCm  = 13;      // <-- closer than this = a side wall, cm
const int manualFrontStopCm = 8;       // <-- closer than this = stop and turn, cm
const int manualFollowCm    = 10;      // <-- how far to stay from the wall, cm

// Do not edit. A 30 cm passage gives side 13, follow 10,
// front 8 on SIMPLE_STEER or 7 on PID_STEER.
const int sideWallCm  = manualDistances ? manualSideWallCm  : corridorWidthCm * 13 / 30;
const int frontStopCm = manualDistances ? manualFrontStopCm : corridorWidthCm * (navigation == PID_STEER ? 7 : 8) / 30;
const int followCm    = manualDistances ? manualFollowCm    : corridorWidthCm * 10 / 30;

const int sensorRangeCm          = 100; // farther than this = no wall, cm
const int sensorMissesBeforeOpen = 2;   // empty pings before that sensor counts as open space
const int exitConfirmLoops       = 2;   // open readings before the maze counts as finished
const int loopDelayMs            = 50;  // pause each loop, ms


// --- Wiring. Change only if your cables differ. ---

const int LEFT_TRIG  = A3;
const int LEFT_ECHO  = A0;
const int FRONT_TRIG = A4;
const int FRONT_ECHO = A1;
const int RIGHT_TRIG = A5;
const int RIGHT_ECHO = A2;

const int LEFT_DIR_A  = 2;   // L298N IN3
const int LEFT_DIR_B  = 3;   // L298N IN4
const int RIGHT_DIR_A = 4;   // L298N IN1
const int RIGHT_DIR_B = 5;   // L298N IN2
const int LEFT_PWM    = 10;  // L298N ENB, PWM
const int RIGHT_PWM   = 11;  // L298N ENA, PWM

const int STATUS_LED = 13;
const int LEFT_LED   = 8;
const int RIGHT_LED  = 9;


// --- Sensors ---
// One object per HC-SR04. The last argument is the farthest ping it will accept.

NewPing sensorLeft (LEFT_TRIG,  LEFT_ECHO,  sensorRangeCm);
NewPing sensorFront(FRONT_TRIG, FRONT_ECHO, sensorRangeCm);
NewPing sensorRight(RIGHT_TRIG, RIGHT_ECHO, sensorRangeCm);

// Latest distance from each sensor, in cm. 0 means no good reading yet.
float leftCm  = 0;
float frontCm = 0;
float rightCm = 0;

// Pings in a row that came back empty.
int leftMisses  = 0;
int frontMisses = 0;
int rightMisses = 0;

// True when a side sensor is closer than sideWallCm, or the front is closer than frontStopCm.
bool wallLeft  = false;
bool wallFront = false;
bool wallRight = false;

// Which wall to follow. FOLLOW_LEFT and FOLLOW_RIGHT are set in setup.
// FOLLOW_AUTO leaves these false until the first front wall.
bool decided     = false;
bool followLeft  = false;
bool followRight = false;

// Exit check. The maze is not finished until a wall has been seen at least once.
int  openLoops  = 0;
bool seenWall   = false;
bool mazeSolved = false;

// PID memory. Cleared on a hard turn so the next steer starts clean.
float pidIntegral = 0;
float pidLastError = 0;
unsigned long pidLastMs = 0;


// --- Sensor reading ---

// One ping. A miss keeps the last good distance, so one bad echo
// does not look like the wall disappeared.
float readDistance(NewPing &sensor, float previous, int &misses) {
  unsigned int raw = sensor.ping_cm();

  if (raw == 0) {
    misses++;
    // Still inside a real wall: hold the last distance for a couple of misses.
    if (previous > 0 && previous < sensorRangeCm && misses < sensorMissesBeforeOpen) {
      return previous;
    }
    // Nothing measured yet. 0 means unknown, not open space.
    if (previous <= 0 && misses < sensorMissesBeforeOpen) return 0;
    return sensorRangeCm;
  }

  misses = 0;

  // A fresh wall after empty space is used as-is. Otherwise blend with the last ping.
  if (previous <= 0 || previous >= sensorRangeCm) return raw;
  return (raw + previous) / 2.0;
}

// Fill leftCm, frontCm, and rightCm.
void readSensors() {
  leftCm  = readDistance(sensorLeft,  leftCm,  leftMisses);
  frontCm = readDistance(sensorFront, frontCm, frontMisses);
  rightCm = readDistance(sensorRight, rightCm, rightMisses);
}

// Turn those distances into wall / no wall.
void findWalls() {
  wallLeft  = (leftCm  > 0 && leftCm  < sideWallCm);
  wallFront = (frontCm > 0 && frontCm < frontStopCm);
  wallRight = (rightCm > 0 && rightCm < sideWallCm);

  if (wallLeft || wallFront || wallRight) seenWall = true;
}


// --- Steering. A positive result means the robot is too far from the wall. ---

// Drop the saved error. Used when a turn starts, so old error does not kick the wheels.
void resetPid() {
  pidIntegral = 0;
  pidLastError = 0;
  pidLastMs = 0;
}

// PID_STEER correction, in PWM. error is cm. Positive means too far from the wall.
float pidStep(float error) {
  unsigned long now = millis();

  if (pidLastMs == 0) {
    pidLastError = error;
    pidLastMs = now;
    return constrain(PID_KP * error, -PID_LIMIT, PID_LIMIT);
  }

  float dt = (now - pidLastMs) / 1000.0;
  if (dt < 0.001) dt = 0.001;

  pidIntegral += error * dt;
  pidIntegral = constrain(pidIntegral, -100.0, 100.0);

  float output = PID_KP * error
               + PID_KI * pidIntegral
               + PID_KD * ((error - pidLastError) / dt);

  pidLastError = error;
  pidLastMs = now;
  return constrain(output, -PID_LIMIT, PID_LIMIT);
}

// PWM to shift the wheels. Positive means the robot is farther from the wall than followCm.
float steeringCorrection(float distance) {
  float error = distance - followCm;
  if (navigation == PID_STEER) return pidStep(error);
  return error * steerStrength;
}


// --- Calibration. Motors stay off. Serial Monitor must be 115200. ---

// Prints the choices and the three distances the robot will actually use.
void printPanel() {
  Serial.println(navigation == PID_STEER ? F("Steering: PID_STEER") : F("Steering: SIMPLE_STEER"));

  Serial.print(F("Wall: "));
  if (strategy == FOLLOW_LEFT)       Serial.println(F("LEFT"));
  else if (strategy == FOLLOW_RIGHT) Serial.println(F("RIGHT"));
  else                               Serial.println(F("AUTO"));

  Serial.print(F("Passage: "));
  Serial.print(corridorWidthCm);
  Serial.println(F(" cm, left wall to right wall"));

  Serial.print(F("Side wall "));
  Serial.print(sideWallCm);
  Serial.print(F(" cm, front stop "));
  Serial.print(frontStopCm);
  Serial.print(F(" cm, follow "));
  Serial.print(followCm);
  Serial.println(F(" cm"));

  if (manualDistances) Serial.println(F("Distances: manual"));
  else                 Serial.println(F("Distances: calculated from the passage"));

  if (sideWallCm < 1 || frontStopCm < 1 || followCm < 1) {
    Serial.println(F("Passage is too narrow. Use 20 cm or more."));
  } else if (followCm >= sideWallCm) {
    Serial.println(F("Follow distance should be smaller than the side wall distance."));
  }
}

// Add one distance to that sensor's min, max, and total. 0 counts as a miss.
void noteSample(int raw, int &minCm, int &maxCm, long &sumCm, int &count, int &misses) {
  if (raw <= 0) {
    misses++;
    return;
  }
  if (count == 0 || raw < minCm) minCm = raw;
  if (raw > maxCm) maxCm = raw;
  sumCm += raw;
  count++;
}

// One sensor row: min, average, max, and how many pings were missed.
void printSensorLine(const __FlashStringHelper *name,
                     int minCm, int maxCm, long sumCm, int count, int misses) {
  Serial.print(name);
  if (count < 1) {
    Serial.print(F("no reading, missed "));
    Serial.println(misses);
    return;
  }
  Serial.print(F("min "));
  Serial.print(minCm);
  Serial.print(F("  avg "));
  Serial.print((int)(sumCm / count));
  Serial.print(F("  max "));
  Serial.print(maxCm);
  Serial.print(F(" cm, missed "));
  Serial.println(misses);
}

// One live line. WALL means closer than sideWallCm, or frontStopCm for the front sensor.
void printDistanceLine(int leftRaw, int frontRaw, int rightRaw) {
  Serial.print(F("L:"));
  if (leftRaw <= 0) Serial.print(F("--"));
  else              Serial.print(leftRaw);
  Serial.print(leftRaw > 0 && leftRaw < sideWallCm ? F(" WALL   ") : F(" open   "));

  Serial.print(F("F:"));
  if (frontRaw <= 0) Serial.print(F("--"));
  else               Serial.print(frontRaw);
  Serial.print(frontRaw > 0 && frontRaw < frontStopCm ? F(" WALL   ") : F(" open   "));

  Serial.print(F("R:"));
  if (rightRaw <= 0) Serial.print(F("--"));
  else               Serial.print(rightRaw);
  Serial.println(rightRaw > 0 && rightRaw < sideWallCm ? F(" WALL") : F(" open"));
}

// Prints the sensor results after the robot has been sitting still and measuring.
// Also prints 3 numbers. To use them, set manualDistances to true and type
// those numbers into manualSideWallCm, manualFrontStopCm, and manualFollowCm.
void printCalibrationReport(
    int leftMin, int leftMax, long leftSum, int leftCount, int leftMiss,
    int frontMin, int frontMax, long frontSum, int frontCount, int frontMiss,
    int rightMin, int rightMax, long rightSum, int rightCount, int rightMiss) {

  Serial.println(F("--- Summary ---"));
  printSensorLine(F("Left   "), leftMin, leftMax, leftSum, leftCount, leftMiss);
  printSensorLine(F("Front  "), frontMin, frontMax, frontSum, frontCount, frontMiss);
  printSensorLine(F("Right  "), rightMin, rightMax, rightSum, rightCount, rightMiss);

  if (leftCount > 0 && rightCount > 0) {
    int airCm = (int)(leftSum / leftCount) + (int)(rightSum / rightCount);
    Serial.print(F("Air between the side sensors is about "));
    Serial.print(airCm);
    Serial.println(F(" cm."));
    Serial.print(F("The robot body fills the rest of the "));
    Serial.print(corridorWidthCm);
    Serial.println(F(" cm passage."));
    if (airCm > corridorWidthCm) {
      Serial.println(F("That is wider than the passage. Check corridorWidthCm, or a sensor is aimed past the wall."));
    }
  } else {
    Serial.println(F("Both side walls were not in range. Put the robot in a passage and run again."));
  }

  int farthest = 0;
  if (leftCount > 0) farthest = leftMax;
  if (rightCount > 0 && rightMax > farthest) farthest = rightMax;

  Serial.print(F("Panel now: side "));
  Serial.print(sideWallCm);
  Serial.print(F(", front "));
  Serial.print(frontStopCm);
  Serial.print(F(", follow "));
  Serial.println(followCm);

  if (farthest > 0) {
    int suggestedSide = farthest + 3;
    if (suggestedSide > sensorRangeCm) suggestedSide = sensorRangeCm;

    long followSum = 0;
    int followCount = 0;
    if (leftCount > 0)  { followSum += leftSum;  followCount += leftCount; }
    if (rightCount > 0) { followSum += rightSum; followCount += rightCount; }
    int suggestedFollow = followCount > 0 ? (int)(followSum / followCount) : followCm;
    if (suggestedFollow >= suggestedSide) suggestedFollow = suggestedSide - 1;
    if (suggestedFollow < 1) suggestedFollow = 1;

    int suggestedFront = frontStopCm;
    if (frontCount > 0) {
      int avgFront = (int)(frontSum / frontCount);
      if (avgFront < sideWallCm && avgFront > 4) suggestedFront = avgFront - 2;
    }

    Serial.println(F("To test the measured passage, set manualDistances true and use:"));
    Serial.print(F("  manualSideWallCm  "));
    Serial.println(suggestedSide);
    Serial.print(F("  manualFrontStopCm "));
    Serial.println(suggestedFront);
    Serial.print(F("  manualFollowCm    "));
    Serial.println(suggestedFollow);
  }
}

// Wheels stay off. Measures for calibrationSeconds, then calls printCalibrationReport.
// After that it keeps printing the live distances so you can move the robot and watch.
void runCalibrationCycle() {
  static bool started = false;
  static bool reported = false;
  static unsigned long startMs = 0;
  static unsigned long lastLine = 0;
  static int leftMin = 0, leftMax = 0, leftCount = 0, leftMiss = 0;
  static int frontMin = 0, frontMax = 0, frontCount = 0, frontMiss = 0;
  static int rightMin = 0, rightMax = 0, rightCount = 0, rightMiss = 0;
  static long leftSum = 0, frontSum = 0, rightSum = 0;

  stopMotors();

  int leftRaw  = sensorLeft.ping_cm();
  int frontRaw = sensorFront.ping_cm();
  int rightRaw = sensorRight.ping_cm();

  if (!started) {
    started = true;
    startMs = millis();
    Serial.println(F("--- Calibration ---"));
    Serial.println(F("Put the robot in a passage, facing along it."));
    Serial.print(F("Collecting for "));
    Serial.print(calibrationSeconds < 1 ? 1 : calibrationSeconds);
    Serial.println(F(" seconds."));
  }

  if (!reported) {
    noteSample(leftRaw, leftMin, leftMax, leftSum, leftCount, leftMiss);
    noteSample(frontRaw, frontMin, frontMax, frontSum, frontCount, frontMiss);
    noteSample(rightRaw, rightMin, rightMax, rightSum, rightCount, rightMiss);

    unsigned long budget = (unsigned long)(calibrationSeconds < 1 ? 1 : calibrationSeconds) * 1000UL;
    if (millis() - startMs >= budget) {
      reported = true;
      printCalibrationReport(
        leftMin, leftMax, leftSum, leftCount, leftMiss,
        frontMin, frontMax, frontSum, frontCount, frontMiss,
        rightMin, rightMax, rightSum, rightCount, rightMiss);
      Serial.println(F("Live readings. WALL means the current distances see a wall."));
    }
  }

  if (millis() - lastLine >= 400) {
    lastLine = millis();
    printDistanceLine(leftRaw, frontRaw, rightRaw);
  }

  heartbeat();
  delay(40);
}


// --- Setup ---

// Set the pins, blink the LEDs once, then print the panel.
void setup() {
  Serial.begin(115200);
  Serial.println(F("STRIKER v3"));

  pinMode(LEFT_DIR_A, OUTPUT);
  pinMode(LEFT_DIR_B, OUTPUT);
  pinMode(RIGHT_DIR_A, OUTPUT);
  pinMode(RIGHT_DIR_B, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);

  pinMode(STATUS_LED, OUTPUT);
  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);

  blinkAllLeds(3);
  stopMotors();

  if (strategy == FOLLOW_LEFT)       lockFollow(true);
  else if (strategy == FOLLOW_RIGHT) lockFollow(false);

  printPanel();
  if (runCalibration) Serial.println(F("Calibration. Motors stay off."));
  else                Serial.println(F("Ready."));
}


// --- Loop ---

// Read, choose a wall if needed, then drive. Calibration replaces all of that.
void loop() {
  if (runCalibration) {
    runCalibrationCycle();
    return;
  }

  readSensors();
  findWalls();

  if (!mazeSolved && !decided && strategy == FOLLOW_AUTO) {
    chooseWallAtFirstTurn();
  }

  drive();
  heartbeat();
  printStatus();
  delay(loopDelayMs);
}


// --- Which wall ---

// Save the chosen wall, light that side's LED, and clear any old steering error.
void lockFollow(bool left) {
  followLeft  = left;
  followRight = !left;
  decided     = true;
  resetPid();

  digitalWrite(LEFT_LED,  left ? HIGH : LOW);
  digitalWrite(RIGHT_LED, left ? LOW  : HIGH);

  if (left) Serial.println(F("Following LEFT wall"));
  else      Serial.println(F("Following RIGHT wall"));
}

// Wait for a wall in front, then follow the side that is open.
void chooseWallAtFirstTurn() {
  if (!wallFront) return;

  if (!wallLeft && wallRight)       lockFollow(true);   // opening on the left
  else if (wallLeft && !wallRight)  lockFollow(false);  // opening on the right
  else                              lockFollow(autoDefault != FOLLOW_RIGHT); // both open, or a dead end
}


// --- Driving ---

// Follow the chosen wall, or creep until one is chosen. Stop when the passage opens up.
void drive() {
  if (mazeSolved) {
    stopMotors();
    return;
  }

  // No walls. Stop immediately so it does not roll off the table.
  if (!wallLeft && !wallFront && !wallRight) {
    stopMotors();
    if (!seenWall) return; // empty pings at startup are not an exit

    openLoops++;
    if (openLoops >= exitConfirmLoops) {
      mazeSolved = true;
      Serial.println(F("Maze solved. Motors stopped."));
    }
    return;
  }

  openLoops = 0;

  // Wall chosen: follow it.
  // Not yet: wait if a wall is in front. With both sides visible, PID_STEER
  // steers toward the nearer one. Otherwise creep forward.
  if (followLeft)       followWall(true);
  else if (followRight) followWall(false);
  else if (wallFront)   stopMotors();
  else if (navigation == PID_STEER && wallLeft && wallRight) centerInCorridor();
  else                  moveForward(driveSpeed / 2);
}

// onLeft true hugs the left wall. false hugs the right wall.
void followWall(bool onLeft) {
  bool sideWall = onLeft ? wallLeft : wallRight;

  // Front wall: turn away from the wall we are following.
  // Side wall gone: turn toward where it should be.
  if (wallFront || !sideWall) {
    resetPid();
    if (wallFront) {
      if (onLeft) turnRight();
      else        turnLeft();
    } else if (onLeft) {
      turnLeft();
    } else {
      turnRight();
    }
    return;
  }

  float distance = onLeft ? leftCm : rightCm;
  float correction = steeringCorrection(distance);
  if (!onLeft) correction = -correction;

  driveMotors(driveSpeed - correction, driveSpeed + correction);
}

// PID_STEER only, before a wall is chosen, and only when both side walls are visible.
// Steers toward the nearer side wall.
void centerInCorridor() {
  int creep = driveSpeed / 2;
  float correction = (rightCm - leftCm) * 2.0;
  driveMotors(creep - correction, creep + correction);
}


// --- Motors ---

// Both wheels forward at the same speed.
void moveForward(int speed) {
  driveMotors(speed, speed);
}

// Both wheels forward. Each speed is PWM, 0 to 255.
void driveMotors(int leftSpeed, int rightSpeed) {
  leftSpeed  = constrain(leftSpeed,  0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);
  digitalWrite(LEFT_DIR_A, HIGH);
  digitalWrite(LEFT_DIR_B, LOW);
  digitalWrite(RIGHT_DIR_A, HIGH);
  digitalWrite(RIGHT_DIR_B, LOW);
  analogWrite(LEFT_PWM, leftSpeed);
  analogWrite(RIGHT_PWM, rightSpeed);
}

// Spin in place. Left wheel reverses, right wheel stays forward.
void turnLeft() {
  int speed = constrain(turnSpeed, 0, 255);
  digitalWrite(LEFT_DIR_A, LOW);
  digitalWrite(LEFT_DIR_B, HIGH);
  digitalWrite(RIGHT_DIR_A, HIGH);
  digitalWrite(RIGHT_DIR_B, LOW);
  analogWrite(LEFT_PWM, speed);
  analogWrite(RIGHT_PWM, speed);
}

// Spin in place. Right wheel reverses, left wheel stays forward.
void turnRight() {
  int speed = constrain(turnSpeed, 0, 255);
  digitalWrite(LEFT_DIR_A, HIGH);
  digitalWrite(LEFT_DIR_B, LOW);
  digitalWrite(RIGHT_DIR_A, LOW);
  digitalWrite(RIGHT_DIR_B, HIGH);
  analogWrite(LEFT_PWM, speed);
  analogWrite(RIGHT_PWM, speed);
}

// Both direction pins low, so the driver lets the motors stop.
void stopMotors() {
  digitalWrite(LEFT_DIR_A, LOW);
  digitalWrite(LEFT_DIR_B, LOW);
  digitalWrite(RIGHT_DIR_A, LOW);
  digitalWrite(RIGHT_DIR_B, LOW);
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}


// --- LEDs and serial ---

// Blink the onboard LED so you can see the program is running.
void heartbeat() {
  static unsigned long last = 0;
  static bool on = false;
  if (millis() - last < 500) return;
  last = millis();
  on = !on;
  digitalWrite(STATUS_LED, on ? HIGH : LOW);
}

// Flash every LED at startup. times is how many flashes.
void blinkAllLeds(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(STATUS_LED, HIGH);
    digitalWrite(LEFT_LED, HIGH);
    digitalWrite(RIGHT_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(LEFT_LED, LOW);
    digitalWrite(RIGHT_LED, LOW);
    delay(200);
  }
}

// Distances and the current wall, about twice a second.
void printStatus() {
  static unsigned long last = 0;
  if (millis() - last < 500) return;
  last = millis();

  Serial.print(navigation == PID_STEER ? F("PID_STEER  ") : F("SIMPLE_STEER  "));
  Serial.print(F("L:"));     Serial.print(leftCm, 0);
  Serial.print(F("cm  F:")); Serial.print(frontCm, 0);
  Serial.print(F("cm  R:")); Serial.print(rightCm, 0);
  Serial.print(F("cm  "));

  if (mazeSolved)           Serial.println(F("MAZE SOLVED, motors stopped"));
  else if (followLeft)      Serial.println(F("following LEFT"));
  else if (followRight)     Serial.println(F("following RIGHT"));
  else                      Serial.println(F("looking for the first turn"));
}
