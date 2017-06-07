# SonoffHM

1.) Voraussetzungen: 
  - installiertes CUxD-Addon auf der CCU
  - ein CUxD Exec-Device (was in den meisten Fällen wohl schon existiert)
    - wie man dieses installiert bitte googlen, es gibt zahlreiche Anleitungen
    - am Ende der ReadMe wird für das Exec-Device der Kanal CUxD.CUX2801001 angenommen. Muss ggf. angepasst werden
  - ein weiteres CUxD Gerät, das wie folgt erstellt wird:
    - Typ = (28) System
    - Funktion = Exec
    - Control = Schalter
    - Name = Sonoff    
      - es wird anschließend in der WebUI ein Gerät namens "Sonoff" mit 16 Kanälen erzeugt. Den ersten Kanal benenennt man um, zB in "Sonoff1"
  - eine Systemvariable, in der die IP des Sonoff gespeichert wird, vom Typ "Zeichenkette"
      - als Namen wählt man 'GeräteName'_IP; Beispiel: Sonoff1_IP ('GeräteName' = der Name des Sonoff-Kanals des CUxD Devices)
  

**Bei Inbetriebnahme des Sonoff blinkt die LED in den ersten 4 Sekunden schnell.
Wird während dessen der Taster *kurz* gedrückt, startet automatisch der Konfigurationsmodus.**
Der Sonoff arbeitet dann als AccessPoint. 
Verbindet man sich mit diesem, startet automatisch eine Konfigurationsseite.

Zum Schluss benötigen wir noch ein kleines Programm, dass den Schaltbefehl an den Sonoff sendet:
- WENN: Geräteauswahl [das CUxD-Device] "bei Schaltzustand: ein" "bei Änderung auslösen"
- DANN: Skript:

  ```string sonoffip = dom.GetObject(dom.GetObject(((dom.GetObject("$src$")).Channel()))#"_IP").Value();
dom.GetObject("CUxD.CUX2801001:1.CMD_EXEC").State("LD_LIBRARY_PATH=/usr/local/addons/cuxd /usr/local/addons/cuxd/curl -s -k http://"#sonoffip#"/1");```
- SONST: Skript:

  ```string sonoffip = dom.GetObject(dom.GetObject(((dom.GetObject("$src$")).Channel()))#"_IP").Value();
dom.GetObject("CUxD.CUX2801001:1.CMD_EXEC").State("LD_LIBRARY_PATH=/usr/local/addons/cuxd /usr/local/addons/cuxd/curl -s -k http://"#sonoffip#"/1")```

- wobei "Sonoff1_IP" der Name der Variable ist, die unter 1. erstellt wurde

# Flashen der Firmware

Wer den Code nicht selbst kompilieren möchte/kann, hat die Möglichkeit, die **'Sonoff.ino.generic.bin'** herunterzuladen und mittels esptool direkt auf den Sonoff zu flashen.
esptool.exe -vv -cd ck -cb 115200 -cp *COMPort* -ca 0x00000 -cf Sonoff.ino.generic.bin

**Eine Anleitung, wie man generell Firmware auf den Sonoff bekommt (Anschluss des FTDI-Interface, Pinbelegung etc), stelle ich hier nicht bereit. Man findet HowTos bei Google wenn man nach "sonoff flash" sucht.**

Der Flash-Vorgang muss nur 1x via FTDI-Kabel erfolgen. Anschließend ist es möglich, den Sonoff via OTA (Over-the-air) flashen.

