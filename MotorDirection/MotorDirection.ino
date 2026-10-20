/*
  STRIKER v3 — motor direction.
  Lift the robot so the wheels are off the ground.
  Each step runs for one second: left, right, then both.
  If a wheel spins backward, swap that motor's two wires. Do not upload again.
  Same pins as Striker_v3. Forward is DIR_A high, DIR_B low.
*/

const int LEFT_DIR_A  = 2;   // L298N IN3
const int LEFT_DIR_B  = 3;   // L298N IN4
const int RIGHT_DIR_A = 4;   // L298N IN1
const int RIGHT_DIR_B = 5;   // L298N IN2
const int LEFT_PWM    = 10;  // L298N ENB
const int RIGHT_PWM   = 11;  // L298N ENA
const int SPEED       = 80;

void setup() {
  pinMode(LEFT_DIR_A, OUTPUT);
  pinMode(LEFT_DIR_B, OUTPUT);
  pinMode(RIGHT_DIR_A, OUTPUT);
  pinMode(RIGHT_DIR_B, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);

  Serial.begin(115200);
  Serial.println("Wheels off the ground.");

  Serial.println("Left wheel forward");
  run(true, false);
  Serial.println("Right wheel forward");
  run(false, true);
  Serial.println("Both wheels forward");
  run(true, true);

  stopMotors();
  Serial.println("Done. A backward wheel means swap that motor's two wires.");
}

void loop() {
}

void run(bool left, bool right) {
  digitalWrite(LEFT_DIR_A, left ? HIGH : LOW);
  digitalWrite(LEFT_DIR_B, LOW);
  digitalWrite(RIGHT_DIR_A, right ? HIGH : LOW);
  digitalWrite(RIGHT_DIR_B, LOW);
  analogWrite(LEFT_PWM, left ? SPEED : 0);
  analogWrite(RIGHT_PWM, right ? SPEED : 0);
  delay(1000);
  stopMotors();
  delay(400);
}

void stopMotors() {
  digitalWrite(LEFT_DIR_A, LOW);
  digitalWrite(LEFT_DIR_B, LOW);
  digitalWrite(RIGHT_DIR_A, LOW);
  digitalWrite(RIGHT_DIR_B, LOW);
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}
