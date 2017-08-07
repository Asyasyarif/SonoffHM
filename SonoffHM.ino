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

#define IPSize 16
#define DeviceNameSize 100

char ccuIP[IPSize]   = "";
char DeviceName[DeviceNameSize] = "";
byte BackendType = 0;

#define greenLEDPin           13
#define RelayPin              12
#define SwitchPin              0
#define MillisKeyBounce      500  //Millisekunden zwischen 2xtasten
#define ConfigPortalTimeout  180  //Timeout (Sekunden) des AccessPoint-Modus
#define HTTPTimeOut         3000  //Timeout (Millisekunden) für http requests

#define BackendType_HomeMatic 0

String bootConfigModeFilename = "bootcfg.mod";

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
char ip[IPSize]      = "0.0.0.0";
char netmask[IPSize] = "0.0.0.0";
char gw[IPSize]      = "0.0.0.0";
bool startWifiManager = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Sonoff startet...");
  pinMode(greenLEDPin, OUTPUT);
  pinMode(RelayPin,    OUTPUT);
  pinMode(SwitchPin,   INPUT_PULLUP);

  Serial.print("Config-Modus durch bootConfigMode aktivieren? ");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/" + bootConfigModeFilename)) {
      startWifiManager = true;
      Serial.println("Ja, " + bootConfigModeFilename + " existiert, starte Config-Modus");
      SPIFFS.remove("/" + bootConfigModeFilename);
      SPIFFS.end();
    } else {
      Serial.println("Nein, " + bootConfigModeFilename + " existiert nicht");
    }
  } else {
    Serial.println("Nein, SPIFFS mount fail!");
  }

  if (!startWifiManager) {
    Serial.println("Config-Modus mit Taster aktivieren?");
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
    Serial.println("Config-Modus " + String(((startWifiManager) ? "" : "nicht ")) + "aktiviert.");
  }

  loadSystemConfig();

  if (doWifiConnect()) {
    Serial.println("WLAN erfolgreich verbunden!");
    printWifiStatus();
  } else ESP.restart();
  server.on("/0", webSwitchRelayOff);
  server.on("/1", webSwitchRelayOn);
  server.on("/2", webToggleRelay);
  server.on("/state", replyRelayState);
  server.on("/bootConfigMode", bootConfigMode);
  server.begin();

  if (BackendType == BackendType_HomeMatic) {
    ChannelName =  "CUxD." + getStateCUxD(DeviceName, "Address");
    digitalWrite(greenLEDPin, HIGH);
    if ((restoreOldState) && (getStateCUxD(ChannelName + ".STATE", "State") == "true")) {
      switchRelayOn(false);
    } else {
      switchRelayOff((getStateCUxD(ChannelName + ".STATE", "State") == "true"));
    }
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

void replyRelayState() {
  server.send(200, "text/plain", "<state>" + String(digitalRead(RelayPin)) + "</state><timer>" + String(TimerSeconds) + "</timer><resttimer>" + String((TimerSeconds > 0) ? (TimerSeconds - (millis() - TimerStartMillis) / 1000) : 0) + "</resttimer>");
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
      if (BackendType == BackendType_HomeMatic) setStateCUxD(ChannelName + ".SET_STATE", "1");
    }
  }
  server.send(200, "text/plain", "<state>" + String(digitalRead(RelayPin)) + "</state><timer>" + String(TimerSeconds) + "</timer>");
}

void switchRelayOff(bool transmitState) {
  Serial.println("switchRelayOff()");
  TimerSeconds = 0;
  RelayState = LOW;
  if (digitalRead(RelayPin) != RelayState) {
    digitalWrite(RelayPin, RelayState);
    digitalWrite(greenLEDPin, !RelayState);
    if (transmitState) {
      if (BackendType == BackendType_HomeMatic) setStateCUxD(ChannelName + ".SET_STATE",  "0" );
    }
  }
  server.send(200, "text/plain", "<state>" + String(digitalRead(RelayPin)) + "</state>");
}

void toggleRelay(bool transmitState) {
  if (digitalRead(RelayPin) == LOW) {
    switchRelayOn(transmitState);
  } else  {
    switchRelayOff(transmitState);
  }
}

void bootConfigMode() {
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (!SPIFFS.exists("/" + bootConfigModeFilename)) {
      File bootConfigModeFile = SPIFFS.open("/" + bootConfigModeFilename, "w");
      bootConfigModeFile.print("0");
      bootConfigModeFile.close();
      SPIFFS.end();
      Serial.println("Boot to ConfigMode requested. Restarting...");
      server.send(200, "text/plain", "<state>enableBootConfigMode - Rebooting</state>");
      delay(500);
      ESP.restart();
    } else {
      server.send(200, "text/plain", "<state>enableBootConfigMode - FAILED!</state>");
      SPIFFS.end();
    }
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
