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
        Serial.println("Content of JSON Config-File: /"+configJsonFile);
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nJSON OK");
          ((json["ip"]).as<String>()).toCharArray(SonoffNetConfig.ip, IPSIZE);
          ((json["netmask"]).as<String>()).toCharArray(SonoffNetConfig.netmask, IPSIZE);
          ((json["gw"]).as<String>()).toCharArray(SonoffNetConfig.gw, IPSIZE);
          ((json["ccuip"]).as<String>()).toCharArray(HomeMaticConfig.ccuIP, IPSIZE);
          ((json["sonoff"]).as<String>()).toCharArray(HomeMaticConfig.DeviceName, DEVICENAMESIZE);

          BackendType = json["backendtype"];
          restoreOldState = json["restoreOldState"];
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
  json["ccuip"] = HomeMaticConfig.ccuIP;
  json["sonoff"] = HomeMaticConfig.DeviceName;
  json["restoreOldState"] = restoreOldState;
  json["backendtype"] = BackendType;

  SPIFFS.remove("/" + configJsonFile);
  File configFile = SPIFFS.open("/" + configJsonFile, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();

  SPIFFS.end();
}

