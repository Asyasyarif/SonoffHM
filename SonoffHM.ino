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

char ccuIP[16]       = "";
char DeviceName[100] = "";

#define greenLEDPin           13
#define RelayPin              12
#define SwitchPin              0
#define MillisKeyBounce      500  //Millisekunden zwischen 2xtasten
#define ConfigPortalTimeout  180  //Timeout des AccessPoint-Modus

bool RelayState = LOW;
bool KeyPress = false;
bool restoreOldState = false;
unsigned long LastMillisKeyPress = 0;
unsigned long TimerStartMillis = 0;
int TimerSeconds = 0;

ESP8266WebServer server(80);
String ChannelName = "";

//WifiManager - don't touch
bool shouldSaveConfig        = false;
String configJsonFile        = "config.json";
#define wifiManagerDebugOutput   true
char ip[16]      = "0.0.0.0";
char netmask[16] = "0.0.0.0";
char gw[16]      = "0.0.0.0";
bool startWifiManager = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Sonoff startet...");
  pinMode(greenLEDPin, OUTPUT);
  pinMode(RelayPin,    OUTPUT);
  pinMode(SwitchPin,   INPUT_PULLUP);

  for (int i = 0; i < 20; i++) {
    if (digitalRead(SwitchPin) == LOW) {
      Serial.println("Config-Modus aktiviert!");
      startWifiManager = true;
      break;
    }
    digitalWrite(greenLEDPin, HIGH);
    delay(100);
    digitalWrite(greenLEDPin, LOW);
    delay(100);
  }

  loadSystemConfig();

  if (doWifiConnect()) {
    Serial.println("WLAN erfolgreich verbunden!");
    printWifiStatus();
  } else ESP.restart();
  server.on("/0", webSwitchRelayOff);
  server.on("/1", webSwitchRelayOn);
  server.on("/2", webToggleRelay);
  server.on("/state", getRelayState);
  server.begin();

  ChannelName =  "CUxD." + getStateFromCCUCUxD(DeviceName, "Address");
  String stateFromCCU = getStateFromCCUCUxD(ChannelName + ".STATE", "State");

  digitalWrite(greenLEDPin, HIGH);

  if (restoreOldState && stateFromCCU == "true") {
    switchRelayOn(false);
  } else {
    switchRelayOff(false);
  }
  startOTAhandling();
}


void loop() {
  if (LastMillisKeyPress > millis())
    LastMillisKeyPress = millis();
  if (TimerStartMillis > millis())
    TimerStartMillis = millis();

  ArduinoOTA.handle();
  server.handleClient();
  if (digitalRead(SwitchPin) == LOW && millis() - LastMillisKeyPress > MillisKeyBounce) {
    LastMillisKeyPress = millis();
    if (KeyPress == false) {
      TimerSeconds = 0;
      toggleRelay(true);
      KeyPress = true;
    }
  } else {
    KeyPress = false;
  }

  if (TimerSeconds > 0 && millis() - TimerStartMillis > TimerSeconds * 1000) {
    Serial.println("Timer abgelaufen. Schalte Relais aus.");
    switchRelayOff(true);
  }

  delay(50);
}

void getRelayState() {
  server.send(200, "text/plain", "<state>" + String(digitalRead(RelayPin)) + "</state><timer>" + String((TimerSeconds > 0) ? (TimerSeconds - (millis() - TimerStartMillis) / 1000) : 0) + "</timer>");
}

void webToggleRelay() {
  toggleRelay(false);
}
void webSwitchRelayOff() {
  switchRelayOff(false);
}

void webSwitchRelayOn() {
  switchRelayOn(false);
  if (server.args() > 0) {
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "t") {
        TimerSeconds = server.arg(i).toInt();
        if (TimerSeconds > 0) {
          TimerStartMillis = millis();
          Serial.println("webSwitchRelayOn(), Timer aktiviert, Sekunden: " + String(TimerSeconds));
        } else {
          Serial.println("webSwitchRelayOn(), Parameter, aber mit TimerSeconds = 0");
        }
      }
    }
  } else {
    TimerSeconds = 0;
    Serial.println("webSwitchRelayOn(), keine Parameter, TimerSeconds = 0");
  }
}

void switchRelayOn(bool transmitState) {
  Serial.println("switchRelayOn()");
  RelayState = HIGH;
  if (digitalRead(RelayPin) != RelayState) {
    digitalWrite(RelayPin, RelayState);
    digitalWrite(greenLEDPin, !RelayState);
    if (transmitState) {
      setStateCCUCUxD(ChannelName + ".SET_STATE", "1");
      server.send(200, "text/plain", "<state>1</state>");
    }
  }
}

void switchRelayOff(bool transmitState) {
  Serial.println("switchRelayOff()");
  TimerSeconds = 0;
  RelayState = LOW;
  if (digitalRead(RelayPin) != RelayState) {
    digitalWrite(RelayPin, RelayState);
    digitalWrite(greenLEDPin, !RelayState);
    if (transmitState) {
      setStateCCUCUxD(ChannelName + ".SET_STATE",  "0" );
      server.send(200, "text/plain", "<state>0</state>");
    }
  }
}

void toggleRelay(bool transmitState) {
  if (digitalRead(RelayPin) == LOW) {
    switchRelayOn(transmitState);
  } else  {
    switchRelayOff(transmitState);
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
