// SPDX-License-Identifier: MIT
#pragma once
#ifndef ESP32
#error "HaldenFunk-MeshCore repeater provisioning currently supports ESP32 only"
#endif
#if LORA_TX_POWER != 10 || !defined(DISABLE_WIFI_OTA)
#error "Unexpected HaldenFunk-MeshCore repeater defaults"
#endif
#if defined(ENABLE_PRIVATE_KEY_EXPORT) || defined(ENABLE_PRIVATE_KEY_IMPORT)
#error "Private key transfer must be disabled"
#endif

// Called after persisted preferences load, before the radio event loop starts.
// No fixed credential is embedded in the published firmware image.
static void disasterRepeaterCredentials(MyMesh &mesh) {
  auto *prefs = mesh.getNodePrefs();
  static_assert(sizeof(prefs->password) >= 15, "Admin password field too short");
  if (!prefs->password[0] || strcmp(prefs->password, "password") == 0) {
    uint8_t random[7];
    esp_fill_random(random, sizeof(random));
    static const char hex[] = "0123456789abcdef";
    for (size_t i=0; i<sizeof(random); ++i) {
      prefs->password[2*i] = hex[random[i] >> 4];
      prefs->password[2*i+1] = hex[random[i] & 15];
    }
    prefs->password[14] = 0;
    mesh.savePrefs();
  }
  Serial.printf("\nHaldenFunk-MeshCore Server\nAdmin-Passwort: %s\nUSB-CLI: 115200 Baud, Zeilenende CR\n", prefs->password);
}
