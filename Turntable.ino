#include <Servo.h>
 
Servo servo;

int detectorPIN = D2;
int servoPIN = D1;
int servoStartPos = 0;
int servoStopPos = 110;

bool pause = false;

void setup() {
  Serial.begin(9600);

  servo.write(servoStartPos);
  servo.attach(servoPIN);

  pinMode(detectorPIN, INPUT);
  
  delay(500);
  raiseStylus();
  releaseStylus();
}

void loop() {
  if (digitalRead(detectorPIN) == HIGH) {
    Serial.println("detected");
    if (!pause) {
      raiseStylus();
      delay(500);
      releaseStylus();
    }
    pause = true;
  } else {
    Serial.print(".");
    pause = false;
  }
  delay(1000);
}

void raiseStylus() {
  for (int pos = servoStartPos; pos <= servoStopPos; pos += 1) { 
    servo.write(pos);
    delay(10);
  }
}

void releaseStylus() {
  servo.write(servoStartPos);
  /*for (int pos = servoStopPos; pos >= servoStartPos; pos -= 1) { 
    servo.write(pos);
    delay(15);
  }*/
}