const char HTTP_CURRENT_STATE_LABEL[] PROGMEM = "<div class='l lt'><label>{v}</label></div><div class='l ls'><label>{ls}</label></div>";
const char HTTP_FW_LABEL[] PROGMEM = "<div class='l c k'><label>Firmware: {fw}</label></div>";
const char HTTP_ONOFF_BUTTONS[] PROGMEM = "<span class='l'><div><button name='btnAction' value='1' type='submit'>AN</button></div><div><table><tr><td>Timer:</td><td align='right'><input class='i' type='text' id='timer' name='timer' length=4 placeholder='Sekunden' value='{ts}'></td></tr></table></div><div><button name='btnAction' value='0' type='submit'>AUS</button></div></span>";
const char HTTP_CONFIG_BUTTON[] PROGMEM = "<div></div><hr /><div></div><div><input class='lnkbtn' type='button' value='Konfiguration' onclick=\"window.location.href='/config'\" /></div>";
const char HTTP_ALL_STYLE[] PROGMEM = "<style>input.lnkbtn {-webkit-appearance: button;-moz-appearance: button;appearance: button;} body {background-color: #303030;} input.lnkbtn,button{color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;padding:5px;} input,button,input.lnkbtn {border: 0;border-radius: 0.3rem;} .c{text-align: center;} .k{font-style:italic;} .fbg {background-color: #eee;} div,input{padding:5px;font-size:1em;} input{width: 95%} .i{text-align: right; width: 40%;} body{text-align: center;font-family:verdana;} .l{no-repeat left center;background-size: 1em;} .q{float: right;width: 64px;text-align: right;} .ls {font-weight: bold;text-align: center;font-size: 300%;} .lt{font-size: 150%;text-align: center;} table{width:100%;} td{max-width:50%;font-weight: bold;} ";
const char HTTP_HM_STYLE[]  PROGMEM = "input.lnkbtn,button{background-color:#1fa3ec;}</style>";
const char HTTP_LOX_STYLE[] PROGMEM = "input.lnkbtn,button{background-color:#83b817;}</style>";
const char HTTP_SAVE_BUTTON[] PROGMEM = "<div><button name='btnSave' value='1' type='submit'>Speichern</button></div><div><input class='lnkbtn' type='button' value='Zur&uuml;ck' onclick=\"window.location.href='/'\" /></div>";
const char HTTP_CONF[] PROGMEM     = "<div class='l lt'><label>{v}</label></div><div><label>{st}:</label></div><div><input type='text' id='ccuip' name='ccuip' length=4 placeholder='{st}' value='{ccuip}'></div><div><label>Ger&auml;tename:</label></div><div><input type='text' id='devicename' name='devicename' length=4 placeholder='Ger&auml;tename' value='{dn}'></div><div><label>Schaltzustand wiederherstellen:</label></div><div><input id='rstate' type='checkbox' name='rstate' {rs} value=1></div>";
const char HTTP_CONF_LOX[] PROGMEM = "<div><label>UDP Port:</label></div><div><input type='text' id='lox_udpport' name='lox_udpport' length=4 placeholder='UDP Port' value='{udp}'></div>";
const char HTTP_CONF_HM[] PROGMEM  = "";
const char HTTP_STATUSLABEL[] PROGMEM = "<div class='l c'>{sl}</div>";

void versionHtml() {
  WebServer.send(200, "text/plain", "<fw>" + FIRMWARE_VERSION + "</fw>");
}

void webSwitchRelayOn() {
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "t") {
        TimerSeconds = WebServer.arg(i).toInt();
        if (TimerSeconds > 0) {
          TimerStartMillis = millis();
          Serial.println("webSwitchRelayOn(), Timer aktiviert, Sekunden: " + String(TimerSeconds));
        } else {
          Serial.println("webSwitchRelayOn(), Parameter, aber mit TimerSeconds = 0");
        }
      }
    }
  } else {
    TimerSeconds = 0;
    Serial.println("webSwitchRelayOn(), keine Parameter, TimerSeconds = 0");
  }
  switchRelay(RELAYSTATE_ON);
  sendDefaultWebCmdReply();
}

void webToggleRelay() {
  toggleRelay(NO_TRANSMITSTATE);
  sendDefaultWebCmdReply();
}
void webSwitchRelayOff() {
  switchRelay(RELAYSTATE_OFF);
  sendDefaultWebCmdReply();
}

void replyRelayState() {
  sendDefaultWebCmdReply();
}

void defaultHtml() {
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnAction")
        switchRelay(WebServer.arg(i).toInt());
      if (WebServer.argName(i) == "timer") {
        TimerSeconds = WebServer.arg(i).toInt();
        if (TimerSeconds > 0) {
          TimerStartMillis = millis();
        }
      }
    }
  }

  String page = FPSTR(HTTP_HEAD);

  //page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += "<div class='fbg'>";

  page += "<form method='post' action='control'>";
  page += FPSTR(HTTP_CURRENT_STATE_LABEL);
  page.replace("{v}", GlobalConfig.DeviceName);

  page.replace("{ls}", ((digitalRead(RelayPin) == HIGH) ? "AN" : "AUS"));

  page += FPSTR(HTTP_ONOFF_BUTTONS);
  page += FPSTR(HTTP_CONFIG_BUTTON);
  String restZeit = "";
  if (TimerSeconds > 0) restZeit =  String(TimerSeconds - (millis() - TimerStartMillis) / 1000) ;
  page.replace("{ts}", restZeit);

  page += FPSTR(HTTP_FW_LABEL);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += "</form></div>";
  page += FPSTR(HTTP_END);
  WebServer.sendHeader("Content-Length", String(page.length()));
  WebServer.send(200, "text/html", page);
}

void configHtml() {
  bool sc = false;
  bool saveSuccess = false;
  bool showHMDevError = false;
  if (WebServer.args() > 0) {
    GlobalConfig.restoreOldRelayState = false;
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnSave")
        sc = (WebServer.arg(i).toInt() == 1);
      if (WebServer.argName(i) == "ccuip")
        strcpy(GlobalConfig.ccuIP, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "devicename")
        strcpy(GlobalConfig.DeviceName, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "lox_udpport")
        strcpy(LoxoneConfig.UDPPort, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "rstate") {
        GlobalConfig.restoreOldRelayState = (String(WebServer.arg(i)).toInt() == 1);
      }

    }
    if (sc) {
      saveSuccess = saveSystemConfig();
      if (GlobalConfig.BackendType == BackendType_HomeMatic) {
        String devName = getStateCUxD(GlobalConfig.DeviceName, "Address") ;
        if (devName != "null") {
          showHMDevError = false;
          HomeMaticConfig.ChannelName =  "CUxD." + devName;
        } else {
          showHMDevError = true;
        }

      }
    }
  }
  String page = FPSTR(HTTP_HEAD);

  //page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += "<div class='fbg'>";
  page += "<form method='post' action='config'>";

  page += FPSTR(HTTP_CONF);
  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    page += FPSTR(HTTP_CONF_HM);
    page.replace("{st}", "CCU2 IP");
  }
  if (GlobalConfig.BackendType == BackendType_Loxone) {
    page += FPSTR(HTTP_CONF_LOX);
    page.replace("{st}", "MiniServer IP");
    page.replace("{udp}", LoxoneConfig.UDPPort);
  }

  page.replace("{rs}", ((GlobalConfig.restoreOldRelayState) ? "checked" : ""));
  page.replace("{dn}", GlobalConfig.DeviceName);
  page.replace("{ccuip}", GlobalConfig.ccuIP);


  page += FPSTR(HTTP_STATUSLABEL);

  if (sc && !showHMDevError) {
    if (saveSuccess) {
      page.replace("{sl}","<label style='color:green'>Speichern erfolgreich.</label>");
    } else {
      page.replace("{sl}","<label style='color:red'>Speichern fehlgeschlagen.</label>");
    }
  }
  
  if (showHMDevError)
    page.replace("{sl}","<label style='color:red'>Ger&auml;tenamen in CUxD pr&uuml;fen!</label>");

  if (!sc && !showHMDevError)
    page.replace("{sl}","");

  page += FPSTR(HTTP_SAVE_BUTTON);
  page += FPSTR(HTTP_FW_LABEL);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += "</form></div>";
  page += FPSTR(HTTP_END);
  page.replace("{v}", GlobalConfig.DeviceName);

  WebServer.send(200, "text/html", page);
}

void sendDefaultWebCmdReply() {
  String reply = createReplyString();
  Serial.println("Sending Web-Reply: " + reply);
  WebServer.send(200, "text/plain", reply);
}

String createReplyString() {
  return "<state>" + String(digitalRead(RelayPin)) + "</state><timer>" + String(TimerSeconds) + "</timer><resttimer>" + String((TimerSeconds > 0) ? (TimerSeconds - (millis() - TimerStartMillis) / 1000) : 0) + "</resttimer><fw>" + FIRMWARE_VERSION + "</fw>";
}

