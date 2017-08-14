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
#include <WiFiUdp.h>

#define IPSIZE         16
#define DEVICENAMESIZE 255
#define UDPPORT        6676

struct hmconfig_t {
  String ChannelName = "";
} HomeMaticConfig;

struct loxoneconfig_t {
  char Username[DEVICENAMESIZE] = "";
  char Password[DEVICENAMESIZE] = "";
  char UDPPort[10] = "";
} LoxoneConfig;

#define greenLEDPin           13
#define RelayPin              12
#define SwitchPin              0
#define MillisKeyBounce      500  //Millisekunden zwischen 2xtasten
#define ConfigPortalTimeout  180  //Timeout (Sekunden) des AccessPoint-Modus
#define HTTPTimeOut         3000  //Timeout (Millisekunden) für http requests

enum BackendTypes_e {
  BackendType_HomeMatic,
  BackendType_Loxone
};

enum RelayStates_e {
  RELAYSTATE_OFF,
  RELAYSTATE_ON
};

enum TransmitStates_e {
  NO_TRANSMITSTATE,
  TRANSMITSTATE
};

struct globalconfig_t {
  char ccuIP[IPSIZE]   = "";
  char DeviceName[DEVICENAMESIZE] = "";
  bool restoreOldRelayState = false;
  bool lastRelayState = false;
  byte BackendType = BackendType_HomeMatic;
} GlobalConfig;

String bootConfigModeFilename = "bootcfg.mod";
String lastRelayStateFilename = "laststat.txt";
bool RelayState = LOW;
bool KeyPress = false;
unsigned long LastMillisKeyPress = 0;
unsigned long TimerStartMillis = 0;
int TimerSeconds = 0;

ESP8266WebServer server(80);

//WifiManager - don't touch
bool shouldSaveConfig        = false;
const String configJsonFile        = "config.json";
#define wifiManagerDebugOutput   true
struct sonoffnetconfig_t {
  char ip[IPSIZE]      = "0.0.0.0";
  char netmask[IPSIZE] = "0.0.0.0";
  char gw[IPSIZE]      = "0.0.0.0";
} SonoffNetConfig;
bool startWifiManager = false;

struct udp_t {
  WiFiUDP UDP;
  char incomingPacket[255];
} UDPClient;

void setup() {
  Serial.begin(115200);
  Serial.println("\nSonoff " + WiFi.macAddress() + " startet...");
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

  if (!loadSystemConfig()) startWifiManager = true;

  if (doWifiConnect()) {
    Serial.println("WLAN erfolgreich verbunden!");
    printWifiStatus();
  } else ESP.restart();
  server.on("/0", webSwitchRelayOff);
  server.on("/off", webSwitchRelayOff);
  server.on("/1", webSwitchRelayOn);
  server.on("/on", webSwitchRelayOn);
  server.on("/2", webToggleRelay);
  server.on("/toggle", webToggleRelay);
  server.on("/state", replyRelayState);
  server.on("/getstate", replyRelayState);
  server.on("/bootConfigMode", setBootConfigMode);
  server.begin();

  GlobalConfig.lastRelayState = getLastState();

  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    HomeMaticConfig.ChannelName =  "CUxD." + getStateCUxD(GlobalConfig.DeviceName, "Address");
    digitalWrite(greenLEDPin, HIGH);
    if ((GlobalConfig.restoreOldRelayState) && GlobalConfig.lastRelayState == true) {
      switchRelay(RELAYSTATE_ON);
    } else {
      switchRelay(RELAYSTATE_OFF, (getStateCUxD(HomeMaticConfig.ChannelName + ".STATE", "State") == "true"));
    }
  }

  if (GlobalConfig.BackendType == BackendType_Loxone) {
    if ((GlobalConfig.restoreOldRelayState) && GlobalConfig.lastRelayState == true) {
      switchRelay(RELAYSTATE_ON);
    } else {
      switchRelay(RELAYSTATE_OFF);
    }
  }
  startOTAhandling();
  UDPClient.UDP.begin(UDPPORT);
}

void loop() {
  //Überlauf der millis() abfangen
  if (LastMillisKeyPress > millis())
    LastMillisKeyPress = millis();
  if (TimerStartMillis > millis())
    TimerStartMillis = millis();

  //auf OTA Anforderung reagieren
  ArduinoOTA.handle();

  //eingehende UDP Kommandos abarbeiten
  String udpMessage = handleUDP();
  if (udpMessage == "bootConfigMode")
    setBootConfigMode;
  if (udpMessage == "1" || udpMessage == "on")
    switchRelay(RELAYSTATE_ON);
  if (udpMessage == "0" || udpMessage == "off")
    switchRelay(RELAYSTATE_OFF);
  if (udpMessage == "2" || udpMessage == "toggle")
    toggleRelay(false);
  if (udpMessage.indexOf("1?t=") != -1) {
    TimerSeconds = (udpMessage.substring(4, udpMessage.length())).toInt();
    if (TimerSeconds > 0) {
      TimerStartMillis = millis();
      Serial.println("webSwitchRelayOn(), Timer aktiviert, Sekunden: " + String(TimerSeconds));
    } else {
      Serial.println("webSwitchRelayOn(), Parameter, aber mit TimerSeconds = 0");
    }
    switchRelay(RELAYSTATE_ON);
  }

  //eingehende HTTP Anfragen abarbeiten
  server.handleClient();

  //Tasterbedienung am Sonoff abarbeiten
  if (digitalRead(SwitchPin) == LOW && millis() - LastMillisKeyPress > MillisKeyBounce) {
    LastMillisKeyPress = millis();
    if (KeyPress == false) {
      TimerSeconds = 0;
      toggleRelay(TRANSMITSTATE);
      KeyPress = true;
    }
  } else {
    KeyPress = false;
  }

  //Timer 
  if (TimerSeconds > 0 && millis() - TimerStartMillis > TimerSeconds * 1000) {
    Serial.println("Timer abgelaufen. Schalte Relais aus.");
    switchRelay(RELAYSTATE_OFF, TRANSMITSTATE);
  }
}

void replyRelayState() {
  server.send(200, "text/plain", "<state>" + String(digitalRead(RelayPin)) + "</state><timer>" + String(TimerSeconds) + "</timer><resttimer>" + String((TimerSeconds > 0) ? (TimerSeconds - (millis() - TimerStartMillis) / 1000) : 0) + "</resttimer>");
}

void webToggleRelay() {
  toggleRelay(NO_TRANSMITSTATE);
}
void webSwitchRelayOff() {
  switchRelay(RELAYSTATE_OFF);
}

void webSwitchRelayOn() {
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
  switchRelay(RELAYSTATE_ON);
}

void switchRelay(bool toState) {
  switchRelay(toState, false);
}
void switchRelay(bool toState, bool transmitState) {
  RelayState = toState;
  Serial.println("Switch Relay to " + String(toState) + " with transmitState = " + String(transmitState));

  if (toState == RELAYSTATE_OFF) {
    TimerSeconds = 0;
  }

  if (transmitState) {
    if (GlobalConfig.BackendType == BackendType_HomeMatic) setStateCUxD(HomeMaticConfig.ChannelName + ".SET_STATE", String(RelayState));
  }

  if (GlobalConfig.BackendType == BackendType_Loxone) sendUDP(String(GlobalConfig.DeviceName) + "=" + String(RelayState));
  digitalWrite(greenLEDPin, !RelayState);
  digitalWrite(RelayPin, RelayState);
  setLastState(RelayState);
  server.send(200, "text/plain", "<state>" + String(RelayState) + "</state><timer>" + String(TimerSeconds) + "</timer>");
}

void toggleRelay(bool transmitState) {
  if (digitalRead(RelayPin) == LOW) {
    switchRelay(RELAYSTATE_ON, transmitState);
  } else  {
    switchRelay(RELAYSTATE_OFF, transmitState);
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
