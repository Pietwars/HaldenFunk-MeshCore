# LILYGO T-Watch S3 – Herkunft (vendored)

Diese Variante stammt aus dem **offenen, noch nicht gemergten** Upstream-Pull-Request
[meshcore-dev/MeshCore#2799](https://github.com/meshcore-dev/MeshCore/pull/2799) „Add LilyGo T-Watch S3 variant“
(Autor djkazic, MIT wie MeshCore). Fest übernommen (vendored) vom PR-Kopf
**`97a595ee9943d4563c7b98810d2377a62b45b899`** (Basis `727fc0512ce08bfd7b499e46daa7fca6eeec730d`), damit der Build
nicht vom PR-Zweig abhängt. CI baut die Uhr ausschließlich aus diesen Dateien im Repository.

Übernommen: `boards/lilygo_twatch_s3.json`, `variants/lilygo_twatch_s3/` (TWatchS3Board.h/.cpp, TWatchS3Display.h,
target.h/.cpp, platformio.ini), `DisplayDriver::getTouch()` in `src/helpers/ui/DisplayDriver.h`.
Nicht übernommen: die Touch-Navigation in `examples/companion_radio/ui-new/UITask.cpp` (HaldenFunk-MeshCore nutzt `ui-brand`, wie DisAHRsterRadio-MeshCore).

Eigene Änderungen gegenüber dem PR:
- Bibliotheken fest: `lewisxhe/XPowersLib @ 0.2.9`, `lovyan03/LovyanGFX @ 1.2.33`, `densaugeo/base64 @ 1.4.0` (statt `^`/`~`).
- `companion_radio_ble` nutzt `examples/companion_radio/ui-brand` (Logo + „Advert senden“).
- Seitentaste: AXP2101-PEKEY-Interrupts (kurz / lang bei 2 s, `pollPowerKey()`), harte Abschaltung 6 s statt 4 s.
- Interrupt-Routinen in `target.cpp` (nicht im Header); `LGFXDisplay`: native Zeichenfunktionen, `LGFX_SPRITE_DEPTH`.
- Profil `dar_LilyGo_TWatchS3_wifi` (WLAN-Portal) in `platformio.local.ini`.

In HaldenFunk-MeshCore 0.9.2 unverändert aus DisAHRsterRadio-MeshCore v0.9.4 übernommen (Akku: `readPower()` über den AXP2101).

Wird der PR upstream gemergt, beim nächsten MeshCore-Update die Dateien mit dem Upstream-Stand abgleichen.
