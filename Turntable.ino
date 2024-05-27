#include <Servo.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

 
Servo servoStylus, servoPower;
ESP8266WebServer webServer(80);
WiFiManager wifiManager;
bool wifiRes;

int detectorPIN = D2;
int servoStylusPIN = D1;
int servoPowerPIN = D3; // WARNING: detach this servo when uploading 

int servoStylusLowerPos = 3;
int servoStylusHigherPos = 140;
int servoPowerRestPos = 0;
int servoPowerOffPos = 180;

bool pause = false;
static unsigned long last_time = 0;

void setup() {
  Serial.begin(9600);

  servoStylus.write(servoStylusLowerPos);
  servoStylus.attach(servoStylusPIN);

  servoPower.write(servoPowerRestPos);
  servoPower.attach(servoPowerPIN);

  pinMode(detectorPIN, INPUT);
  
  delay(500);
  pausePlaying();
  delay(500);
  powerOff();
  delay(1000);/**/

  WiFi.mode(WIFI_STA);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("TurntableAP", "TurntableAPPassword");

  while (!MDNS.begin("Turntable")) {
    Serial.println("Error setting up MDNS responder!");
    delay(500);
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  webServer.on("/pause", getPause);
  webServer.on("/stop", getStop);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
}

void loop() {
  MDNS.update();
  webServer.handleClient();

  if(millis() - last_time > 1000) {
    if (digitalRead(detectorPIN) == HIGH) {
      Serial.println("detected");
      if (!pause) {
        pausePlaying();
        powerOff();
      }
      pause = true;
    } else {
      Serial.print(".");
      pause = false;
    }
    last_time = millis();
  }
}

void pausePlaying() {
  raiseStylus();
  delay(500);
  releaseStylus();
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

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(wifiManager.getConfigPortalSSID());
}

void getPause() {
  Serial.println("API pause");
  pausePlaying();
  webServer.send(200);
}

void getStop() {
  Serial.println("API stop");
  pausePlaying();
  powerOff();
  webServer.send(200);
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