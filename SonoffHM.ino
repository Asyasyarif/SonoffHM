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

char ssid[] =           "XXXXXX";
char key[] =            "XXXXXX";

String ccuIP =          "192.168.1.1";
String DeviceName =     "Sonoff1";

String DeviceIP_Variable = DeviceName + "_IP";

#define greenLEDPin 13
#define RelayPin    12
#define SwitchPin   0

bool relayState = LOW;
bool keyPress = false;
char OTAHostname[] = "Sonoff-OTA";
ESP8266WebServer server(80);
String ChannelName = "";
void setup() {
  pinMode(greenLEDPin, OUTPUT);
  pinMode(RelayPin,    OUTPUT);
  pinMode(SwitchPin,   INPUT);

  Serial.begin(9600);
  if (doWifiConnect() == true) {
    startOTAhandling();
    if (!setStateCCUCUxD(DeviceIP_Variable, "'" + WiFi.localIP().toString() + "'")) {
      Serial.println("Error setting Variable " + DeviceIP_Variable);
      ESP.restart();
    }
    server.on("/0", switchRelayOff);
    server.on("/1", switchRelayOn);
    server.on("/2", toggleRelay);
    server.on("/state", returnRelayState);
    server.begin();
  }

  ChannelName =  "CUxD." + getStateFromCCUCUxD(DeviceName, "Address");
  String stateFromCCU = getStateFromCCUCUxD(ChannelName + ".STATE", "State");

  if (stateFromCCU == "true") {
    switchRelayOn();
  } else {
    switchRelayOff();
  }
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

bool doWifiConnect() {
  Serial.println("Connecting WLAN...");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, key);
  int waitCounter = 0;
  digitalWrite(greenLEDPin, LOW);
  while (WiFi.status() != WL_CONNECTED) {
    waitCounter++;
    if (waitCounter == 20) {
      digitalWrite(greenLEDPin, HIGH);
      return false;
    }
    delay(500);
  }
  digitalWrite(greenLEDPin, HIGH);
  Serial.println("Wifi Connected");
  return true;
}

void blinkLED(int count) {
  pinMode(greenLEDPin, OUTPUT);

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
