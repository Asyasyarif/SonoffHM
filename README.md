# SonoffHM

1.) Voraussetzungen: 
  - installiertes CUxD- und XML-API-Addon auf der CCU
  - ein CUxD Exec-Device (was in den meisten Fällen wohl schon existiert)
    - wie man dieses installiert bitte googlen, es gibt zahlreiche Anleitungen
  - ein CUxD Gerät, das wie folgt erstellt wird:
    - Typ = (28) System
    - Funktion = Exec
    - Control = Schalter
  - eine Systemvariable, in der die IP des Sonoff gespeichert wird, vom Typ "Zeichenkette", wegen der Übersichtlichkeit am besten dem Gerätekanal des neuen CUxD Device-Kanals zugeordnet
      - als Namen nimmt man zB Sonoff1_IP
  
In der SonoffHM.ino müssen noch folgende Variablen angepasst werden:
  - String ccuIP =          "192.168.1.1";           
    - die IP der CCU
  - String ChannelName =    "CUxD.CUX2801002:1";
    - der Gerätekanal des neu angelegten CUxD-Devices
  - String ise_Sonoff1_IP = "00000";
    - die ID der Systemvariablen für die IP
    - man findet sie über die XML-API:
      - http://ccu2/config/xmlapi/sysvarlist.cgi
      - -> Auf der Seite nach dem Variablennamen suchen und in der Zeile nach "ise_id" suchen. Die Zahlen, die dort folgen, ist die benötigte ID
  - String ise_Device =     "00000";
    - die ID des CUxD-Devices
    - man findet sie auch über die XML-API:
      - http://ccu2/config/xmlapi/statelist.cgi
      - -> Auf der Seite nach dem Kanalnamen.STATE suchen (zB "CUxD.CUX2801002:1.STATE"). Anschließend ist in dieser Zeile dann auch wieder die "ise_id" zu finden
  - char ssid[] =           "XXXXXX";
    - die SSID des WLANs
  - char key[] =            "XXXXXX";
    - der WPA2-Key des WLANS


Zum Schluss benötigen wir noch ein kleines Programm, dass den Schaltbefehl an den Sonoff sendet:
- WENN: Geräteauswahl [das CUxD-Device] "bei Schaltzustand: ein" "bei Änderung auslösen"
- DANN: Skript:
  - var sonoff1ip = dom.GetObject("Sonoff1_IP").Value();
dom.GetObject("CUxD.CUX2801001:1.CMD_EXEC").State("LD_LIBRARY_PATH=/usr/local/addons/cuxd /usr/local/addons/cuxd/curl -s -k http://"#sonoff1ip#"/1");
- SONST: Skript:
  - var sonoff1ip = dom.GetObject("Sonoff1_IP").Value();
dom.GetObject("CUxD.CUX2801001:1.CMD_EXEC").State("LD_LIBRARY_PATH=/usr/local/addons/cuxd /usr/local/addons/cuxd/curl -s -k http://"#sonoff1ip#"/0");

- wobei "Sonoff1_IP" der Name der Variable ist, die unter 1. erstellt wurde


