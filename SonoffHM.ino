#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>

// BENUTZERVARIABLEN:
char ssid[] = "Apple Network";
char key[] = "L00A05712F8D7";
int refreshSeconds = 60;
uint16_t scrollPause = 5000;
String url = "http://192.168.1.251:8087/getPlainValue/javascript.0.PoolMatrixDisplay.valueString";
uint8_t scrollSpeed = 24;

textEffect_t scrollEffectIn  = PA_SCROLL_LEFT;
textEffect_t scrollEffectOut = PA_SCROLL_UP;
textPosition_t scrollAlign   = PA_CENTER;

//fixe Display
#define MAX_DEVICES 8
#define CLK_PIN   D7
#define DATA_PIN  D5
#define CS_PIN    D8
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
uint8_t degC[] = {5, 6 , 15 , 9 , 15 , 6 };
uint8_t line[] = {4,  0, 8, 8, 8 };
uint8_t plus[] = {5, 8, 8, 62, 8, 8};
uint8_t block[] =  {3, 255, 255, 255};
char curMessage[75];

int loopCount = 0;
int cnt = 0;
long lastMillis = 0;
String valueArray[20];
int valueCount = 0;
int intensity = 0;
int modeCnt = 0;

const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE]; 
unsigned int localPort = 2390;
const char* ntpServerName = "time.nist.gov";
WiFiUDP udp;

#define key1 D3
#define key2 D4
bool key1last = false;
bool key2last = false;

void setup()
{
  Serial.begin(115200);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
  } else {
    if (!loadConfig()) {
      Serial.println("Failed to load config");
    } else {
      Serial.println("Config loaded");
    }
  }

  pinMode(key1, INPUT_PULLUP);
  pinMode(key2, INPUT_PULLUP);
  P.begin();
  P.setIntensity(intensity);
  P.displayClear();
  P.displaySuspend(false);
  P.addChar('$', degC);
  P.addChar('-', line);
  P.addChar('_', block);

  if (doWifiConnect() == true) {
    udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(3600);
    while (timeStatus() == timeNotSet) {
      Serial.println("Waiting for Time set");
      delay(2000);
    }
    P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffectIn, scrollEffectOut);
  }
  else ESP.restart();
}

void loop()
{
  if (digitalRead(key1) == LOW ) {
    if (!key1last) {
      key1last = true;
      intensity = intensity + 2;
      if (intensity > 14) intensity = 0;
      P.setIntensity(intensity);
      Serial.println("Set intensity to " + String(intensity));
      if (!saveConfig()) {
        Serial.println("Failed to save config");
      } else {
        Serial.println("Config saved");
      }
      delay(50);
    }
  } else {
    key1last = false;
  }

  if (digitalRead(key2) == LOW ) {
    if (!key2last) {
      key2last = true;
      modeCnt++;
      if (modeCnt > valueCount + 1) modeCnt = 0;
      Serial.println("Mode: " + String(modeCnt));
      delay(50);
    }
  } else {
    key2last = false;
  }

  if ((millis() - lastMillis > refreshSeconds * 1000) || lastMillis == 0) {
    Serial.println("Hole Werte von ioBroker...");
    String valueString = loadDataFromURL();
    char buf[valueString.length() + 1];
    valueString.toCharArray(buf, sizeof(buf));
    char *p = buf;
    char *str;
    valueCount = 0;
    while ((str = strtok_r(p, ";", &p)) != NULL) {
      valueArray[valueCount] = str;
      valueCount++;
    }
    lastMillis = millis();
  }

  if (modeCnt == 0) {
    if (P.displayAnimate())
    {
      loopCount++;
      if (loopCount > valueCount || valueCount == 0) {
        String Zeit = calcTime(now());
        Zeit.toCharArray(curMessage, 10);
        loopCount = -1;
      } else {
        String currentValue = valueArray[loopCount];
        currentValue.toCharArray(curMessage, currentValue.length() + 1);
      }
      P.displayReset();
      P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffectIn, scrollEffectOut);
    }
  } else {
    if (valueCount > 0) {
      String currentValue = valueArray[modeCnt - 1];
      currentValue.toCharArray(curMessage, currentValue.length() + 1);
    }
    if (modeCnt > valueCount) {
      String Zeit = calcTime(now());
      Zeit.toCharArray(curMessage, 10);
    }

    P.displayText(curMessage, PA_CENTER, scrollSpeed, 10, PA_PRINT, PA_PRINT);
    P.displayAnimate();
  }
}
