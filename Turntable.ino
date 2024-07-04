#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "icons-48.c"
#include "Font5x7Fixed.h"

#define OLED_RESET     -1 
 
Servo servoStylus, servoSpeedSelector;
ESP8266WebServer webServer(80);
Adafruit_SSD1306 display(OLED_RESET);
WiFiManager wifiManager;
bool wifiRes;

int sensorDiskEndedPIN = D8;
int servoStylusPIN = D7;
int servoSpeedSelectorPIN = D3; // WARNING: detach this servo when uploading 
int releSelfPowerPIN = D0;
int buttonBarPIN = A0;

int servoStylusLowerPos = 3;
int servoStylusHigherPos = 140;
int servoSpeedSelector45Pos = 31;
int servoSpeedSelectorOffPos = 90;
int servoSpeedSelector33Pos = 149;
int servoSpeedSelectorCurrentPos = servoSpeedSelectorOffPos;

String turntableStatus = "stop";
String turntableSpeed = "0";

bool pause = false;
bool canTurnOff = true;
static unsigned long last_time = 0;
static unsigned long start_time_stanby_ckeck = 0;

// PLAY PAUSE STOP FUNCTIONS

void pausePlaying(bool canDrawIcon=true) {
  if (canDrawIcon) {
    display.clearDisplay();
    display.drawBitmap(0, 0, icon_pause_64, 64, 48, WHITE);
    display.display();
  }
  raiseStylus();
  delay(500);
  releaseStylus();
  turntableStatus = "pause";
}

void setup() {
  Serial.begin(115200);

  // Keep power on
  pinMode(releSelfPowerPIN, OUTPUT);
  selfPowerOn();
  
  // Display 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setFont(&Font5x7Fixed);
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,9);             // Start at top-left corner
  display.println("Booting...");
  display.display();

  // Optical sensor
  pinMode(sensorDiskEndedPIN, INPUT);

  // Servo stylus init
  servoStylus.write(servoStylusLowerPos);
  servoStylus.attach(servoStylusPIN);

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
  // webServer.on("/play/45", httpGETPlay45);
  // webServer.on("/play/33", httpGETPlay33);
  webServer.on("/play", httpGETPlay);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  display.clearDisplay();
  pausePlaying(false);

  // but at boot is turntable is stopped
  display.clearDisplay();
  display.drawBitmap(0, 0, icon_stop_64, 64, 48, WHITE);
  display.display();
  canTurnOff = true;
  turntableStatus = "stop";
  turntableSpeed = "0";
  start_time_stanby_ckeck = millis();
}

void loop() {
  // every loop
  MDNS.update();
  webServer.handleClient();

  // every 1s
  if(millis() - last_time > 1000) {
    float voltage = analogRead(buttonBarPIN) * (3.3 / 1023.0);
    Serial.print("Analog value ");
    Serial.println(voltage);
    if (voltage < 0.8) {
      // nothing
    }
    if (voltage >= 0.8 && voltage < 1) {
      // stop
      pausePlaying();
      stopSpin();
    }
    if (voltage >= 1 && voltage < 2) {
      // pause
      pausePlaying();
    }
    if (voltage >= 2 && voltage < 3) {
      // 45
      startSpin("45");
    }
    if (voltage >= 3) {
      // 33
      startSpin("33");
    }

    if (digitalRead(sensorDiskEndedPIN) == HIGH) {
      Serial.println("End of record detected");
      if (!pause) {
        pausePlaying();
        stopSpin();
        // selfPowerOff();
      }
      pause = true;
    } else {
      Serial.print(".");
      pause = false;
    }
    last_time = millis();
  }

  // turn off is not used
  if (millis() - start_time_stanby_ckeck > 60000) {
    if (canTurnOff && turntableStatus == "stop") {
      selfPowerOff();
    }
  }
}

// STYLUS FUNCTIONS

void raiseStylus() {
  for (int pos = servoStylusLowerPos; pos <= servoStylusHigherPos; pos += 1) { 
    servoStylus.write(pos);
    delay(7);
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
  display.clearDisplay();
  display.drawBitmap(0, 0, icon_stop_64, 64, 48, WHITE);
  display.display();
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
  canTurnOff = true;
  start_time_stanby_ckeck = millis();
}

void startSpin(String speed) {
  display.clearDisplay();
  display.setCursor(53,7);             
  display.println(speed);
  display.drawBitmap(0, 0, icon_play_64, 64, 48, WHITE);
  display.display();
  int targetPosition = ((speed == "33") ? servoSpeedSelector33Pos : servoSpeedSelector45Pos);
  Serial.print("Start spinning to ");
  Serial.println(speed);
  servoSpeedSelectorToPosition(targetPosition);
  turntableStatus = "play";
  turntableSpeed = speed;
  canTurnOff = false;
}

// POWER FUNCTIONS

void selfPowerOn() {
  digitalWrite(releSelfPowerPIN, 1);
}

void selfPowerOff() {
  display.clearDisplay();
  display.setCursor(0,7);             
  display.println("bye!");
  display.display();
  delay(1000);
  digitalWrite(releSelfPowerPIN, 0);
}

// WEBSERVER FUNCTIONS

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

/*void httpGETPlay45() {
  Serial.println("API play 45");
  startSpin("45");
  webServer.send(200, "application/json", jsonTurntableStatus());
}

void httpGETPlay33() {
  Serial.println("API play 33");
  startSpin("33");
  webServer.send(200, "application/json", jsonTurntableStatus());
}*/

void httpGETPlay() {
  String speed = "33";
  if (webServer.hasArg("speed")) speed = webServer.arg("speed");
  Serial.println("API play " + speed);
  startSpin(speed);
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