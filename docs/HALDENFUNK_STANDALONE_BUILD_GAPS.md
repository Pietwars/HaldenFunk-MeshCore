# HaldenFunk-MeshCore Standalone – Entwicklungsstand

Branch: `dev/align-disahr-095`

## Umgesetzt

- Standalone-Unterbau der DisAHRsterRadio-MeshCore-Vorlage 0.9.5/test6 für LILYGO T-Deck Plus
- lokale Bedienung für Chat, Schnellmeldungen, Brett, Karte, Standort und Geräteinfo
- Touchscreen, Tastatur/Trackball, Akkuanzeige, GPS, SD-Karte und Bluetooth-Companion
- Standard-Bluetooth-PIN mit voller MeshCore-App-Funktion nach PIN-Pairing
- keine Admin-Oberfläche im Standalone
- Kupfer/Anthrazit-Farbwelt sowie HaldenFunk-Karten, Orte und Bootgrafik
- MeshCore-Funkprofil bleibt von der LoRaLayer2-Firmware getrennt

## Automatische Nachweise

GitHub Actions erzeugt regionale Assets, führt Hosttests aus, baut `HaldenFunk_TDeck_standalone` mit PlatformIO und prüft, dass keine WLAN-/Web-Schicht gelinkt ist. Ergebnisse gelten erst mit einem grünen Workflow-Lauf als bestanden.

## Offen

- sämtliche realen Hardwaretests auf einem LILYGO T-Deck Plus
- Review und Freigabe; kein Merge, Tag oder Release ohne ausdrückliche Zustimmung
