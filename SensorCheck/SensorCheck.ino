/*
  STRIKER v3 — sensor check.
  Motors stay off. Open the Serial Monitor at 115200.
  Same pins as Striker_v3.
*/

#include <NewPing.h>

const int LEFT_TRIG  = A3;
const int LEFT_ECHO  = A0;
const int FRONT_TRIG = A4;
const int FRONT_ECHO = A1;
const int RIGHT_TRIG = A5;
const int RIGHT_ECHO = A2;
const int RANGE_CM   = 100;

NewPing sensorLeft (LEFT_TRIG,  LEFT_ECHO,  RANGE_CM);
NewPing sensorFront(FRONT_TRIG, FRONT_ECHO, RANGE_CM);
NewPing sensorRight(RIGHT_TRIG, RIGHT_ECHO, RANGE_CM);

void setup() {
  Serial.begin(115200);
  Serial.println("cm   left  front  right");
}

void loop() {
  unsigned int left  = sensorLeft.ping_cm();
  delay(30);
  unsigned int front = sensorFront.ping_cm();
  delay(30);
  unsigned int right = sensorRight.ping_cm();

  Serial.print(left);
  Serial.print("  ");
  Serial.print(front);
  Serial.print("  ");
  Serial.println(right);
  delay(200);
}
