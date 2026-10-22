# HaldenFunk T-Deck Plus Standalone: verified build gaps

Target branch: `dev/align-disahr-095`. Source baseline: `Pietwars/DisAHRsterRadio-MeshCore` 0.9.5, not upstream MeshCore.

## Verified missing on HaldenFunk development branch

- `examples/companion_radio/StandaloneFrontend.cpp`
- `examples/companion_radio/ui-brand/StandaloneUI.cpp`
- `examples/companion_radio/ui-brand/assets/tdeck-splash.rgb565`

All three paths exist in the DisAHRsterRadio-MeshCore source repository (the splash asset is binary). HaldenFunk `platformio.standalone.ini` does not currently declare `board_build.embed_files` for the splash asset.

## Required before claiming a successful build

1. Port complete standalone frontend, local companion dependencies, UI headers, rendering and supporting modules from DisAHRsterRadio 0.9.5; check transitive includes and compile guards.
2. Port or replace splash binary with HaldenFunk artwork, then add correct `board_build.embed_files` configuration.
3. Preserve standard MeshCore PIN pairing; remove Admin button, menu and password dependencies; preserve HaldenFunk radio and regional configuration.
4. Run `pio run -e HaldenFunk_TDeck_standalone`, inspect errors and linked output; run standalone checks and hardware tests.

No build, hardware verification, merge or release is asserted by this document.
