bool doWifiConnect() {
  String _ssid = WiFi.SSID();
  String _psk = WiFi.psk();

  const char* ipStr = ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  WiFiManager wifiManager;
  digitalWrite(greenLEDPin, LOW);
  wifiManager.setDebugOutput(wifiManagerDebugOutput);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", ccuIP, 16);
  WiFiManagerParameter custom_sonoffname("sonoff", "Sonoff DeviceName", DeviceName, 100);
  char*chrRestoreOldState = "0";
  if (restoreOldState) chrRestoreOldState =  "1" ;                   
  WiFiManagerParameter custom_cbrestorestate("restorestate", "Restore state:", chrRestoreOldState, 5, 1);
  //WiFiManagerParameter custom_usedbackend("usedbackend", "Backend", "", 10, 2, "<option value=\"0\">CCU</option><option selected value=\"1\">ioBroker</option>");

  WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", "", 16);
  WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", "", 16);
  WiFiManagerParameter custom_gw("custom_gw", "Gateway", "", 16);
  WiFiManagerParameter custom_text("<br/><br>Statische IP (wenn leer, dann DHCP):");
  wifiManager.addParameter(&custom_ccuip);
  wifiManager.addParameter(&custom_sonoffname);
  wifiManager.addParameter(&custom_cbrestorestate);
  //wifiManager.addParameter(&custom_usedbackend);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_netmask);
  wifiManager.addParameter(&custom_gw);

  String strHostname = "Sonoff-" + WiFi.macAddress();
  char* Hostname = &strHostname[0];

  wifiManager.setConfigPortalTimeout(ConfigPortalTimeout);

  if (startWifiManager == true) {
    if (_ssid == "" || _psk == "" ) {
      wifiManager.resetSettings();
    }
    else {
      if (!wifiManager.startConfigPortal(Hostname)) {
        Serial.println("failed to connect and hit timeout");
        delay(1000);
        ESP.restart();
      }
    }
  }

  wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

  wifiManager.autoConnect(Hostname);

  Serial.println("Wifi Connected");
  Serial.println("CUSTOM STATIC IP: " + String(ip) + " Netmask: " + String(netmask) + " GW: " + String(gw));
  if (shouldSaveConfig) {
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

    restoreOldState = (atoi(custom_cbrestorestate.getValue()) == 1);
    
    strcpy(ccuIP, custom_ccuip.getValue());
    strcpy(DeviceName, custom_sonoffname.getValue());

    saveSystemConfig();

    delay(100);
    ESP.restart();
  }

  return true;
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
  Serial.println("WLAN Info:");
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

