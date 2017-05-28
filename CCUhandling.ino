void setStateCCUCUxD(String id, String value) {
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.setTimeout(5000);
    String url = "http://" + ccuIP + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + ".SET_STATE%22).State(" + value + ")";
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("HTTP " + id + " success");
      String payload = http.getString();
    }
    if (httpCode != 200) {
      blinkLED(3);
      Serial.println("HTTP " + id + " fail");
    }
    http.end();
  } else ESP.restart();

}

void setStateCCUXMLAPI(String ise_id, String value) {
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + ccuIP + "/config/xmlapi/statechange.cgi?ise_id=" + ise_id + "&new_value=" + value;
    http.begin(url);
    int httpCode = http.GET();
    Serial.println("httpcode = " + String(httpCode));
    if (httpCode > 0) {
      //     String payload = http.getString();
    }
    if (httpCode != 200) {
      Serial.println("HTTP fail " + String(httpCode));
    }
    http.end();
  } else ESP.restart();
}

String getStateFromCCUXMLAPI(String ise_id) {
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.setTimeout(5000);
    http.begin("http://" + ccuIP + "/config/xmlapi/state.cgi?datapoint_id=" + ise_id);
    int httpCode = http.GET();
    String payload = "error";
    if (httpCode > 0) {
      payload = http.getString();
    }
    if (httpCode != 200) {
      blinkLED(3);
      Serial.println("HTTP " + ise_id + " fail");
    }
    http.end();


    payload = payload.substring(payload.indexOf("value='"));
    payload = payload.substring(7, payload.indexOf("'/>"));

    return payload;
  } else ESP.restart();

}
