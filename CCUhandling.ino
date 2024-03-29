bool setStateCUxD(String id, String value) {
  if (id.indexOf(".null.") == -1) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      String url = "http://" + String(HomeMaticConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22).State(" + value + ")";
      Serial.print("setStateCCUCUxD url: " + url + " -> ");
      http.begin(url);
      int httpCode = http.GET();
      String payload = "";

      if (httpCode > 0) {
        Serial.println("HTTP " + id + " success");
        payload = http.getString();
      }
      if (httpCode != 200) {
        blinkLED(3);
        Serial.println("HTTP " + id + " fail");
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));
      Serial.println("result: " + payload);

      return (payload != "null");

    } else ESP.restart();
  } else return true;
}

String getStateCUxD(String id, String type) {
  if (id.indexOf(".null.") == -1) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      String url = "http://" + String(HomeMaticConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22)." + type + "()";
      Serial.print("getStateFromCCUCUxD url: " + url + " -> ");
      http.begin(url);
      int httpCode = http.GET();
      String payload = "error";
      if (httpCode > 0) {
        payload = http.getString();
      }
      if (httpCode != 200) {
        blinkLED(3);
        Serial.println("HTTP " + id + " fail");
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));
      Serial.println("result: " + payload);
      return payload;
    } else ESP.restart();
  } else return "";
}

