# Änderungen

## 0.1.0-alpha · 17.10.2026 · erste Vorabversion HaldenFunk-MeshCore

Vorabversion, auf keinem Board abgenommen. Kein Release.

- Neues Repository, abgeleitet von [DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) 0.9.0-alpha (Git-Historie erhalten): **MeshCore 1.18**, WLAN-Weboberfläche + Bluetooth für die offizielle MeshCore-App, Gerätematrix der MeshCore-Konfigurator-Liste.
- Regionales Design und Inhalte aus [HaldenFunk](https://github.com/Pietwars/HaldenFunk): Kupfer/Anthrazit-Farben, Hintergrundbild (Halden, Förderturm, Funkwellen), App-Symbol, Name **HaldenFunk**, Untertitel „Glück Auf – Mesh fürs Mansfelder Land“.
- Ortsliste Mansfelder Land (Sangerhausen, Hohe Linde, Eisleben, Mansfeld, Hettstedt, Allstedt, Wallhausen, Kelbra, Roßla, Kyffhäuser, Stolberg, Wippra) und 5 Offline-Karten (OSM, ODbL) statt Ahrtal.
- WLAN-Name `HaldenFunk-XXXXXX` mit geräteindividuellem Zufallspasswort; Knotennamen `HaldenFunk-…`; Firmwarekennung `v1.18.0-hf0.1.0-alpha`; Paketnamen `HaldenFunk-MeshCore-0.1.0-alpha-<Gerät>.zip`.
- Live-Demo mit Beispieldaten aus dem Mansfelder Land: https://pietwars.github.io/HaldenFunk-MeshCore/
- README: Projektfamilie inkl. HaldenFunk und HaldenFunk-MeshCore; Hinweis: nicht funkkompatibel mit HaldenFunk/DisAHRsterRadio (LoRaLayer2).

**Im Notfall immer 112.**

---

Historie der Basis DisAHRsterRadio-MeshCore:

### 0.9.0-alpha-Nachtrag · 17.10.2026 · Vereinheitlichung

- README: Abschnitt „Projektfamilie DisAHRsterRadio“ (in allen drei Repositorys identisch) mit Versionen, Live-Demos, Release, Flyern und Namensregel (DisAHRsterRadio = Projekt/Marke, MeshAhrtal = Weboberfläche, DisasterMesh = Original-Repo).
- Vorschaubilder aller sechs Bereiche neu aus der Live-Demo aufgenommen (Handygröße, aktuelles Vollbild-Design) als `docs/images/funktion-*.jpg`; alte Bilder `darmc-*.png` und PNG-Fassungen entfernt.
- Flyer `docs/flyer/DisAHRsterRadio-MeshCore-Flyer.pdf` (Druck, QR-Codes zu Release und Demo) und `…-Flyer-digital.pdf` (klickbare Links zu Repository, Release, Live-Demo, Projektseite); in der Live-Demo unter `/flyer/` verlinkt.
- Boardtabelle mit Flash-Belegung der 3-MB-App-Partition, Build-Stand 105 Geräte.

### 0.9.0-alpha · 16.10.2026 · erste Vorabversion

[Release v0.9.0-alpha](https://github.com/Pietwars/DisAHRsterRadio-MeshCore/releases/tag/v0.9.0-alpha) – Vorabversion, auf keinem Board abgenommen.

- MeshCore auf **1.18.0** (Upstream `main`, Commit `81dbae59`), Firmwarekennung `v1.18.0-dar0.9.0-alpha`.
- Builds für alle 105 Geräte der MeshCore-Konfigurator-Liste (202 Builds): ESP32-Geräte mit WLAN-Portal (36), nRF52/RP2040/STM32 als Standard-Companion, Repeater und Room-Server ohne Web-Oberfläche; Ausnahmen mit Begründung in der Gerätetabelle (ThinkNode M7, M5Stack Unit C6L, ThinkNode M8 Repeater/Room).
- WLAN-Portal und Bluetooth für die offizielle MeshCore-App gleichzeitig; zufällige Bluetooth-PIN pro Gerät (seriell/OLED).
- Partition `huge_app.csv` (3 MB App); `factory.bin` einmalig an Adresse 0x0 flashen.
- Heltec V2: gestaffelter WLAN-Start gegen Brownout (ungeprüft).
- Weboberfläche MeshAhrtal (kompakt): Chat, Schnellmeldung, Brett, Karte, Standort, Gerät; Hintergrundbild in voller Breite.
- Live-Demo auf GitHub Pages: https://pietwars.github.io/DisAHRsterRadio-MeshCore/

**Im Notfall immer 112.**
