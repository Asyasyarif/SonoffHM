/*
  Generic ESP8266 Module
  Flash Mode: QIO
  Flash Frequency: 40 MHz
  CPU Frequency: 80 MHz
  Flash Size: 1M (256K SPIFFS)
  Debug Port: disabled
  Debug Level: none
  Reset Mode: ck
  Upload Speed: 115200
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>

char ccuIP[15]      = "0.0.0.0";
char DeviceName[20] = "";

#define greenLEDPin 13
#define RelayPin    12
#define SwitchPin   0

bool relayState = LOW;
bool keyPress = false;

ESP8266WebServer server(80);
String ChannelName = "";

//WifiManager - don't touch
bool shouldSaveConfig        = false;
String configJsonFile        = "config.json";
bool wifiManagerDebugOutput  = true;
char ip[15]      = "0.0.0.0";
char netmask[15] = "0.0.0.0";
char gw[15]      = "0.0.0.0";
bool startWifiManager = false;

void setup() {
  pinMode(greenLEDPin, OUTPUT);
  pinMode(RelayPin,    OUTPUT);
  pinMode(SwitchPin,   INPUT);

  for (int i = 0; i < 20; i++) {
    if (digitalRead(SwitchPin) == LOW) {
      startWifiManager = true;
      break;
    }
    digitalWrite(greenLEDPin, HIGH);
    delay(100);
    digitalWrite(greenLEDPin, LOW);
    delay(100);
  }

  loadSystemConfig();

  Serial.begin(9600);
  if (doWifiConnect()) {
    startOTAhandling();
    if (!setStateCCUCUxD(String(DeviceName) + "_IP", "'" + WiFi.localIP().toString() + "'")) {
      Serial.println("Error setting Variable " + String(DeviceName) + "_IP");
      ESP.restart();
    }
    server.on("/0", switchRelayOff);
    server.on("/1", switchRelayOn);
    server.on("/2", toggleRelay);
    server.on("/state", returnRelayState);
    server.begin();

    ChannelName =  "CUxD." + getStateFromCCUCUxD(DeviceName, "Address");
    String stateFromCCU = getStateFromCCUCUxD(ChannelName + ".STATE", "State");

    digitalWrite(greenLEDPin, HIGH);

    if (stateFromCCU == "true") {
      switchRelayOn();
    } else {
      switchRelayOff();
    }
  }
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  if (digitalRead(SwitchPin) == LOW) {
    if (keyPress == false) {
      toggleRelay();
      keyPress = true;
    }
  } else {
    keyPress = false;
  }
  delay(10);
}

void returnRelayState() {
  server.send(200, "text/html", String(digitalRead(RelayPin)));
}

void switchRelayOn() {
  relayState = HIGH;
  server.send(200, "text/html", "On OK");
  if (digitalRead(RelayPin) != relayState) {
    digitalWrite(RelayPin, relayState);
    digitalWrite(greenLEDPin, !relayState);
    setStateCCUCUxD(ChannelName + ".SET_STATE", "1");
  }
}

void switchRelayOff() {
  relayState = LOW;
  server.send(200, "text/html", "Off OK");
  if (digitalRead(RelayPin) != relayState) {
    digitalWrite(RelayPin, relayState);
    digitalWrite(greenLEDPin, !relayState);
    setStateCCUCUxD(ChannelName + ".SET_STATE",  "0" );
  }
}

void toggleRelay() {
  if (digitalRead(RelayPin) == LOW) {
    switchRelayOn();
  } else  {
    switchRelayOff();
  }
}

void blinkLED(int count) {
  digitalWrite(greenLEDPin, LOW);
  delay(100);
  for (int i = 0; i < count; i++) {
    digitalWrite(greenLEDPin, HIGH);
    delay(100);
    digitalWrite(greenLEDPin, LOW);
    delay(100);
  }
  delay(200);
}
