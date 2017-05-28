/*
  Generic ESP8266 Module
  Flash Mode: QIO
  Flash Frequency: 40 MHz
  CPU Frequency: 80 MHz
  Flash Size: 1M (64K SPIFFS)
  Debug Port: disabled
  Debug Level: none
  Reset Mode: ck
  Upload Speed: 115200
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

String ccuIP =          "192.168.1.1";
String ChannelName =    "CUxD.CUX2801003:1";
String ise_Sonoff1_IP = "00000";
String ise_Device =     "00000";
char ssid[] =           "XXXXXX";
char key[] =            "XXXXXX";


#define greenLEDPin 13
#define RelayPin    12
#define SwitchPin   0

bool relayState = LOW;
bool keyPress = false;

ESP8266WebServer server(80);

void setup() {
  pinMode(greenLEDPin, OUTPUT);
  pinMode(RelayPin,    OUTPUT);
  pinMode(SwitchPin,   INPUT);

  Serial.begin(9600);
  if (doWifiConnect() == true) {
    setStateCCUXMLAPI(ise_Sonoff1_IP, WiFi.localIP().toString());
    server.on("/0", switchRelayOff);
    server.on("/1", switchRelayOn);
    server.on("/2", toggleRelay);
    server.on("/state", returnRelayState);
    server.begin();
  }

  String stateFromCCU = getStateFromCCUXMLAPI(ise_Device);

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
    setStateCCUCUxD(ChannelName, "1");
  }
}

void switchRelayOff() {
  relayState = LOW;
  server.send(200, "text/html", "Off OK");
  if (digitalRead(RelayPin) != relayState) {
    digitalWrite(RelayPin, relayState);
    digitalWrite(greenLEDPin, !relayState);
    setStateCCUCUxD(ChannelName,  "0" );
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
