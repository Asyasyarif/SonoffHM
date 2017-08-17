bool loadSystemConfig() {
  Serial.println("mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/" + configJsonFile)) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        Serial.println("Content of JSON Config-File: /" + configJsonFile);
        json.printTo(Serial);
        Serial.println();
        if (json.success()) {
          Serial.println("\nJSON OK");
          ((json["ip"]).as<String>()).toCharArray(SonoffNetConfig.ip, IPSIZE);
          ((json["netmask"]).as<String>()).toCharArray(SonoffNetConfig.netmask, IPSIZE);
          ((json["gw"]).as<String>()).toCharArray(SonoffNetConfig.gw, IPSIZE);
          ((json["ccuip"]).as<String>()).toCharArray(GlobalConfig.ccuIP, IPSIZE);
          ((json["sonoff"]).as<String>()).toCharArray(GlobalConfig.DeviceName, DEVICENAMESIZE);
          //((json["loxusername"]).as<String>()).toCharArray(LoxoneConfig.Username, DEVICENAMESIZE);
          //((json["loxpassword"]).as<String>()).toCharArray(LoxoneConfig.Password, DEVICENAMESIZE);
          ((json["loxudpport"]).as<String>()).toCharArray(LoxoneConfig.UDPPort, 10);

          GlobalConfig.BackendType = json["backendtype"];
          GlobalConfig.restoreOldRelayState = json["restoreOldState"];
        } else {
          Serial.println("\nERROR loading config");
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

bool saveSystemConfig() {
  SPIFFS.begin();
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["ip"] = SonoffNetConfig.ip;
  json["netmask"] = SonoffNetConfig.netmask;
  json["gw"] = SonoffNetConfig.gw;
  json["ccuip"] = GlobalConfig.ccuIP;
  json["sonoff"] = GlobalConfig.DeviceName;
  json["restoreOldState"] = GlobalConfig.restoreOldRelayState;
  json["backendtype"] = GlobalConfig.BackendType;
  //json["loxusername"] = LoxoneConfig.Username;
  //json["loxpassword"] = LoxoneConfig.Password;
  json["loxudpport"] = LoxoneConfig.UDPPort;


  SPIFFS.remove("/" + configJsonFile);
  File configFile = SPIFFS.open("/" + configJsonFile, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  Serial.println();
  json.printTo(configFile);
  configFile.close();

  SPIFFS.end();
}

void setLastState(bool state) {
  GlobalConfig.lastRelayState = state;
  if (GlobalConfig.restoreOldRelayState) {
    if (SPIFFS.begin()) {
      Serial.println("setLastState mounted file system");
      //SPIFFS.remove("/" + lastStateFilename);
      File setLastStateFile = SPIFFS.open("/" + lastRelayStateFilename, "w");
      setLastStateFile.print(state);
      setLastStateFile.close();
      SPIFFS.end();
      Serial.println("setLastState (" + String(state) + ") saved.");
    } else {
      Serial.println("setLastState SPIFFS mount fail!");
    }
  }
}

bool getLastState() {
  if (GlobalConfig.restoreOldRelayState) {
    if (SPIFFS.begin()) {
      Serial.println("getLastState mounted file system");
      if (SPIFFS.exists("/" + lastRelayStateFilename)) {
        Serial.println(lastRelayStateFilename + " existiert");
        File lastStateFile = SPIFFS.open("/" + lastRelayStateFilename, "r");
        bool bLastState = false;
        if (lastStateFile && lastStateFile.size()) {
          String content = String(char(lastStateFile.read()));
          Serial.println("getLastState FileContent = " + content);
          bLastState = (content == "1");
        }
        SPIFFS.end();
        return bLastState;
      } else {
        Serial.println(lastRelayStateFilename + " existiert nicht");
      }
    } else {
      Serial.println("getLastState SPIFFS mount fail!");
      false;
    }
  } else {
    return false;
  }
}

void setBootConfigMode() {
  if (SPIFFS.begin()) {
    Serial.println("setBootConfigMode mounted file system");
    if (!SPIFFS.exists("/" + bootConfigModeFilename)) {
      File bootConfigModeFile = SPIFFS.open("/" + bootConfigModeFilename, "w");
      bootConfigModeFile.print("0");
      bootConfigModeFile.close();
      SPIFFS.end();
      Serial.println("Boot to ConfigMode requested. Restarting...");
      WebServer.send(200, "text/plain", "<state>enableBootConfigMode - Rebooting</state>");
      delay(500);
      ESP.restart();
    } else {
      WebServer.send(200, "text/plain", "<state>enableBootConfigMode - FAILED!</state>");
      SPIFFS.end();
    }
  }
}

