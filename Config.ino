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
          ((json["ip"]).as<String>()).toCharArray(ip, IPSize);
          ((json["netmask"]).as<String>()).toCharArray(netmask, IPSize);
          ((json["gw"]).as<String>()).toCharArray(gw, IPSize);
          ((json["ccuip"]).as<String>()).toCharArray(ccuIP, IPSize);
          ((json["sonoff"]).as<String>()).toCharArray(DeviceName, DeviceNameSize);

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
  json["ip"] = ip;
  json["netmask"] = netmask;
  json["gw"] = gw;
  json["ccuip"] = ccuIP;
  json["sonoff"] = DeviceName;
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

