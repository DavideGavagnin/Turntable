#include <Servo.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

 
Servo servoStylus, servoSpeedSelector;
ESP8266WebServer webServer(80);
WiFiManager wifiManager;
bool wifiRes;

int sensorDiskEndedPIN = D2;
int servoStylusPIN = D1;
int servoSpeedSelectorPIN = D3; // WARNING: detach this servo when uploading 
int releSelfPowerPIN = D0;

int servoStylusLowerPos = 3;
int servoStylusHigherPos = 140;
int servoSpeedSelector45Pos = 31;
int servoSpeedSelectorOffPos = 90;
int servoSpeedSelector33Pos = 149;
int servoSpeedSelectorCurrentPos = servoSpeedSelectorOffPos;

String turntableStatus = "stop";
String turntableSpeed = "0";

bool pause = false;
static unsigned long last_time = 0;

void setup() {
  Serial.begin(115200);

  // Keep power on
  pinMode(releSelfPowerPIN, OUTPUT);
  selfPowerOn();
  // Optical sensor
  pinMode(sensorDiskEndedPIN, INPUT);

  // Servo stylus init
  servoStylus.write(servoStylusLowerPos);
  servoStylus.attach(servoStylusPIN);
  pausePlaying();

  // Servo speed selector init
  servoSpeedSelectorToPosition(servoSpeedSelectorOffPos);  

  // Network init
  WiFi.mode(WIFI_STA);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("TurntableAP", "TurntableAPPassword");

  while (!MDNS.begin("Turntable")) {
    Serial.println("Error setting up MDNS responder!");
    delay(500);
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  webServer.on("/pause", httpGETPause);
  webServer.on("/stop", httpGETStop);
  webServer.on("/status", httpGETStatus);
  webServer.on("/play45", httpGETPlay45);
  webServer.on("/play33", httpGETPlay33);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
}

void loop() {
  // every loop
  MDNS.update();
  webServer.handleClient();

  // every 1s
  if(millis() - last_time > 1000) {
    if (digitalRead(sensorDiskEndedPIN) == HIGH) {
      Serial.println("End of record detected");
      if (!pause) {
        pausePlaying();
        stopSpin();
        selfPowerOff();
      }
      pause = true;
    } else {
      Serial.print(".");
      pause = false;
    }
    last_time = millis();
  }
}

// PLAY PAUSE STOP FUNCTIONS

void pausePlaying() {
  raiseStylus();
  delay(500);
  releaseStylus();
  turntableStatus = "pause";
}

// STYLUS FUNCTIONS

void raiseStylus() {
  for (int pos = servoStylusLowerPos; pos <= servoStylusHigherPos; pos += 1) { 
    servoStylus.write(pos);
    delay(10);
  }
}

void releaseStylus() {
  servoStylus.write(servoStylusLowerPos);
}

// RECORD SPIN FUNCIONS

void servoSpeedSelectorToPosition(int position) {
  Serial.print("Servo speed selector from ");
  Serial.print(servoSpeedSelectorCurrentPos);
  Serial.print(" to ");
  Serial.println(position);
  servoSpeedSelector.write(servoSpeedSelectorCurrentPos);
  servoSpeedSelector.attach(servoSpeedSelectorPIN);
  if (servoSpeedSelectorCurrentPos < position) {
    for (int pos = servoSpeedSelectorCurrentPos; pos < position; pos += 1) { 
      servoSpeedSelector.write(pos);
      delay(10);
    }
  } 
  if (servoSpeedSelectorCurrentPos > position) {
    for (int pos = servoSpeedSelectorCurrentPos; pos > position; pos -= 1) { 
      servoSpeedSelector.write(pos);
      delay(10);
    }
  }
  servoSpeedSelector.write(position);
  delay(200);
  servoSpeedSelectorCurrentPos = position;
  servoSpeedSelector.detach();
}

void stopSpin() {
  int targetPosition = servoSpeedSelectorOffPos;
  Serial.print("Stop spinning from ");
  Serial.println(turntableSpeed);
  if (turntableSpeed == "45") {
    targetPosition += 5;
  } 
  if (turntableSpeed == "33") {
    targetPosition -= 5;
  }
  servoSpeedSelectorToPosition(targetPosition);
  servoSpeedSelectorToPosition(servoSpeedSelectorOffPos);
  turntableStatus = "stop";
  turntableSpeed = "0";
}

void startSpin(String speed) {
  int targetPosition = ((speed == "33") ? servoSpeedSelector33Pos : servoSpeedSelector45Pos);
  Serial.print("Start spinning to ");
  Serial.println(speed);
  servoSpeedSelectorToPosition(targetPosition);
  turntableStatus = "play";
  turntableSpeed = speed;
}

// POWER FUNCTIONS

void selfPowerOn() {
  digitalWrite(releSelfPowerPIN, 1);
}

void selfPowerOff() {
  delay(500);
  digitalWrite(releSelfPowerPIN, 0);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(wifiManager.getConfigPortalSSID());
}

void httpGETStatus() {
  Serial.println("API status");
  webServer.send(200, "application/json", jsonTurntableStatus());
}

void httpGETPause() {
  Serial.println("API pause");
  pausePlaying();
  webServer.send(200, "application/json", jsonTurntableStatus());
}

void httpGETStop() {
  Serial.println("API stop");
  pausePlaying();
  stopSpin();
  delay(500);
  webServer.send(200, "application/json", jsonTurntableStatus());
  selfPowerOff();
}

void httpGETPlay45() {
  Serial.println("API play 45");
  startSpin("45");
  webServer.send(200, "application/json", jsonTurntableStatus());
}

void httpGETPlay33() {
  Serial.println("API play 33");
  startSpin("33");
  webServer.send(200, "application/json", jsonTurntableStatus());
}

void handleNotFound() {
  Serial.println(webServer.uri() + "404: Not found");
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
}

String jsonTurntableStatus() {
  return "{\"status\":\"" + turntableStatus + "\",\"speed\":\"" + turntableSpeed + "\"}";
}