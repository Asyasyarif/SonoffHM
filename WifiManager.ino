bool doWifiConnect() {
  WiFiManager wifiManager;
  digitalWrite(greenLEDPin, LOW);
  wifiManager.setDebugOutput(wifiManagerDebugOutput);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", "", 15);
  WiFiManagerParameter custom_sonoffname("sonoff", "Sonoff DeviceName", "", 20);

  WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", "", 15);
  WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", "", 15);
  WiFiManagerParameter custom_gw("custom_gw", "Gateway", "", 15);
  WiFiManagerParameter custom_text("<br/><br>Statische IP (wenn leer, dann DHCP):");
  wifiManager.addParameter(&custom_ccuip);
  wifiManager.addParameter(&custom_sonoffname);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_netmask);
  wifiManager.addParameter(&custom_gw);

  if (startWifiManager == true) {
    wifiManager.resetSettings();
  }
  const char* ipStr = ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

  String Hostname = "Sonoff-" + WiFi.macAddress();
  char a[] = "";
  Hostname.toCharArray(a, 30);

  wifiManager.autoConnect(a);

  Serial.println("Wifi Connected");
  Serial.println("CUSTOM STATIC IP: " + String(ip) + " Netmask: " + String(netmask) + " GW: " + String(gw));
  if (shouldSaveConfig) {
    SPIFFS.begin();
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    if (String(custom_ip.getValue()).length() > 5) {
      Serial.println("Custom IP Address is set!");
      strcpy(ip, custom_ip.getValue());
      strcpy(netmask, custom_netmask.getValue());
      strcpy(gw, custom_gw.getValue());
    } else {
      strcpy(ip,      "0.0.0.0");
      strcpy(netmask, "0.0.0.0");
      strcpy(gw,      "0.0.0.0");
    }
    strcpy(ccuIP, custom_ccuip.getValue());
    strcpy(DeviceName, custom_sonoffname.getValue());
    json["ip"] = ip;
    json["netmask"] = netmask;
    json["gw"] = gw;
    json["ccuip"] = ccuIP;
    json["sonoff"] = DeviceName;

    SPIFFS.remove("/" + configJsonFile);
    File configFile = SPIFFS.open("/" + configJsonFile, "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();

    SPIFFS.end();
    delay(100);
    ESP.restart();
  }

  return true;
}


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("AP-Modus ist aktiv!");
  //Ausgabe, dass der AP Modus aktiv ist
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                            // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}

bool loadSystemConfig() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/" + configJsonFile)) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(ip,      json["ip"]);
          strcpy(netmask, json["netmask"]);
          strcpy(gw,      json["gw"]);
          strcpy(ccuIP,   json["ccuip"]);
          strcpy(DeviceName,      json["sonoff"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
      return true;
    } else {
      Serial.println("/" + configJsonFile + " not found.");
      return false;
    }
    SPIFFS.end();
  } else {
    Serial.println("failed to mount FS");
    return false;
  }
}
