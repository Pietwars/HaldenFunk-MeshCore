#pragma once
// Schnellmeldung on the T-Deck Plus standalone display (Issue #35, taken over from 0.9.5-test #33/#34):
// the web UI's quick messages and Ahrtal places (QuickData.h, generated from web/index.html), keyboard input,
// device GPS only with a fresh fix and only after "Standort teilen" was switched on.
#if defined(LILYGO_TDECK) && defined(DISASTER_STANDALONE)
#include "DeviceApi.h"
#include "QuickModel.h"
#include "TouchLayout.h"

#include <helpers/ui/DisplayDriver.h>

class TDeckQuickPage {
  quick::Model model;
  dev::Api* api = nullptr;
  char target[14] = "c0", targetName[64] = "# Oeffentlich";
  char status[96] = "Vorlage, Ort und Ziel antippen / Tastatur: Text";
  unsigned targetIndex = 0;
  bool submitted = false, lastFix = false, placeChosen = false;
  long lastSatellites = -1;
  uint32_t nextGps = 0, nextStatus = 0;
  int picker = -1, pickerTop = 0, pickerSel = 0, pickerRows = 0;
  char pickerIds[touchui::VISIBLE_ROWS][14] = {};
  unsigned choiceCount() const;
  void openPicker(int which);
  void choose(int index);
  void renderPicker(DisplayDriver *d);
  void selectTarget(int direction);
  bool refreshTargetName();
  void draftChanged() {
    submitted = false;
    snprintf(status, sizeof(status), "Entwurf / nur Senden loest eine Nachricht aus");
  }
  void submit(bool position, uint32_t stamp);
  static void displayText(DisplayDriver* d, int x, int y, uint16_t color, const char* text, int limit = 52);
  static bool quickTarget(const dev::Api& a, unsigned index, char* id, size_t id_size, char* name, size_t name_size);
public:
  unsigned unreadChat = 0, unreadBoard = 0;   // unread total in the status line
  void begin(dev::Api* a) { api = a; enter(); }
  // On opening the page: default place "Mein Standort" only while sharing is on, else "Ohne Standort"
  // (until the user picks a place himself).
  void enter() { if (!placeChosen) model.place = api && api->gps().share ? 0 : -1; }
  quick::Fix gpsFix() const;
  bool activate(uint32_t stamp); // true=request return to the main menu
  bool key(uint8_t key, uint32_t stamp);
  // Trackball: up/down = previous/next field; left/right = change the selection of the field.
  // Returns false if left/right had nothing to change.
  bool move(int dx, int dy);
  // Touch tap on a preset, the text box, "Leeren", Ort, Ziel or the two send buttons (same as focus + Enter).
  // Returns true only for the Home action (never for a tap, the status line is the caller's back zone).
  bool tap(int x, int y, uint32_t stamp);
  int touchTarget(int x, int y) const;
  bool swipe(int dy);
  bool closePicker() {
    if (picker < 0) return false;
    picker = -1;
    return true;
  }
  bool picking() const { return picker >= 0; }
  bool textFocus() const { return model.focus == quick::Text; }
  void setTarget(const char* id);             // preselect the chat currently open (like the web)
  bool tick(uint32_t now);
  void render(DisplayDriver* d);
  const quick::Model& draft() const { return model; }
  const char* statusText() const { return status; }
};
#endif
