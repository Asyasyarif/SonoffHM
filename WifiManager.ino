bool doWifiConnect() {

  String _ssid = WiFi.SSID();
  String _psk = WiFi.psk();

  String _pskMask = "";
  for (int i = 0; i < _psk.length(); i++) {
    _pskMask += "*";
  }
  Serial.println("ssid = " + _ssid + ", psk = " + _pskMask);


  const char* ipStr = SonoffNetConfig.ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = SonoffNetConfig.netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = SonoffNetConfig.gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  if (!startWifiManager && _ssid != "" && _psk != "" ) {
    Serial.println("Connecting WLAN the classic way...");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid.c_str(), _psk.c_str());
    int waitCounter = 0;
    if (String(SonoffNetConfig.ip) != "0.0.0.0")
      WiFi.config(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

    while (WiFi.status() != WL_CONNECTED) {
      waitCounter++;
      Serial.print(".");
      if (waitCounter == 20) {
        return false;
      }
      delay(500);
    }
    Serial.println("Wifi Connected");
    return true;
  } else {
    WiFiManager wifiManager;
    digitalWrite(greenLEDPin, LOW);
    wifiManager.setDebugOutput(wifiManagerDebugOutput);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", HomeMaticConfig.ccuIP, IPSIZE);
    WiFiManagerParameter custom_sonoffname("sonoff", "Sonoff DeviceName", HomeMaticConfig.DeviceName, DEVICENAMESIZE);
    char*chrRestoreOldState = "0";
    if (restoreOldState) chrRestoreOldState =  "1" ;
    WiFiManagerParameter custom_cbrestorestate("restorestate", "Schaltzustand wiederherstellen: ", chrRestoreOldState, 8, 1);
    WiFiManagerParameter custom_backendtype("backendtype", "Backend", "", 8, 2, "<option selected value='0'>HomeMatic</option>");

    WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", (String(SonoffNetConfig.ip) != "0.0.0.0") ? SonoffNetConfig.ip : "", IPSIZE);
    WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", (String(SonoffNetConfig.netmask) != "0.0.0.0") ? SonoffNetConfig.netmask : "", IPSIZE);
    WiFiManagerParameter custom_gw("custom_gw", "Gateway",  (String(SonoffNetConfig.gw) != "0.0.0.0") ? SonoffNetConfig.gw : "", IPSIZE);
    WiFiManagerParameter custom_text("<br/><br>Statische IP (wenn leer, dann DHCP):");
    wifiManager.addParameter(&custom_ccuip);
    wifiManager.addParameter(&custom_sonoffname);
    wifiManager.addParameter(&custom_cbrestorestate);
    wifiManager.addParameter(&custom_backendtype);
    wifiManager.addParameter(&custom_text);
    wifiManager.addParameter(&custom_ip);
    wifiManager.addParameter(&custom_netmask);
    wifiManager.addParameter(&custom_gw);

    String Hostname = "Sonoff-" + WiFi.macAddress();

    wifiManager.setConfigPortalTimeout(ConfigPortalTimeout);

    if (startWifiManager == true) {
      if (_ssid == "" || _psk == "" ) {
        wifiManager.resetSettings();
      }
      else {
        if (!wifiManager.startConfigPortal(Hostname.c_str())) {
          Serial.println("failed to connect and hit timeout");
          delay(500);
          ESP.restart();
        }
      }
    }

    wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

    wifiManager.autoConnect(Hostname.c_str());

    Serial.println("Wifi Connected");
    Serial.println("CUSTOM STATIC IP: " + String(SonoffNetConfig.ip) + " Netmask: " + String(SonoffNetConfig.netmask) + " GW: " + String(SonoffNetConfig.gw));
    if (shouldSaveConfig) {
      if (String(custom_ip.getValue()).length() > 5) {
        Serial.println("Custom IP Address is set!");
        strcpy(SonoffNetConfig.ip, custom_ip.getValue());
        strcpy(SonoffNetConfig.netmask, custom_netmask.getValue());
        strcpy(SonoffNetConfig.gw, custom_gw.getValue());

      } else {
        strcpy(SonoffNetConfig.ip,      "0.0.0.0");
        strcpy(SonoffNetConfig.netmask, "0.0.0.0");
        strcpy(SonoffNetConfig.gw,      "0.0.0.0");
      }

      restoreOldState = (atoi(custom_cbrestorestate.getValue()) == 1);
      BackendType = (atoi(custom_backendtype.getValue()));

      strcpy(HomeMaticConfig.ccuIP, custom_ccuip.getValue());
      strcpy(HomeMaticConfig.DeviceName, custom_sonoffname.getValue());

      saveSystemConfig();
      
      delay(100);
      ESP.restart();
    }
    return true;
  }
}


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("AP-Modus ist aktiv!");
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);
    str = strchr(str, sep);
    if (str == NULL || *str == '\0') {
      break;
    }
    str++;
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

