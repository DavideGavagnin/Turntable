#include <Servo.h>
 
Servo servoStylus, servoPower;

int detectorPIN = D2;
int servoStylusPIN = D1;
int servoPowerPIN = D3; // WARNING: detach this servo when uploading 

int servoStylusLowerPos = 3;
int servoStylusHigherPos = 140;
int servoPowerRestPos = 0;
int servoPowerOffPos = 180;

bool pause = false;

void setup() {
  Serial.begin(9600);

  servoStylus.write(servoStylusLowerPos);
  servoStylus.attach(servoStylusPIN);

  servoPower.write(servoPowerRestPos);
  servoPower.attach(servoPowerPIN);

  pinMode(detectorPIN, INPUT);
  
  delay(500);
  raiseStylus();
  releaseStylus();
  delay(500);
  powerOff();
  delay(1000);/**/
}

void loop() {
  if (digitalRead(detectorPIN) == HIGH) {
    Serial.println("detected");
    if (!pause) {
      raiseStylus();
      delay(500);
      releaseStylus();
      powerOff();
    }
    pause = true;
  } else {
    Serial.print(".");
    pause = false;
  }
  delay(1000);/**/
}

void raiseStylus() {
  for (int pos = servoStylusLowerPos; pos <= servoStylusHigherPos; pos += 1) { 
    servoStylus.write(pos);
    delay(10);
  }
}

void releaseStylus() {
  servoStylus.write(servoStylusLowerPos);
  /*for (int pos = servoStylusHigherPos; pos >= servoStylusLowerPos; pos -= 1) { 
    servoStylus.write(pos);
    delay(15);
  }*/
}

void powerOff() {
  for (int pos = servoPowerRestPos; pos <= servoPowerOffPos; pos += 1) { 
    servoPower.write(pos);
    delay(10);
  }
  powerRest();
}

void powerRest() {
  servoPower.write(servoPowerRestPos);
}