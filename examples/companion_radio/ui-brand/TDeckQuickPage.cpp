#if defined(LILYGO_TDECK) && defined(DISASTER_STANDALONE)
#include "TDeckQuickPage.h"
#include "Widgets.h"

namespace {
// Web design system (Theme.h): .quick-btn.H wine, .I blue, .A mesh green on card background.
constexpr uint16_t BG = theme::BG, WHITE = theme::TEXT, GREEN = theme::MESH, GREY = theme::MUTED;
constexpr uint16_t HELP = theme::rgb(0x5a2733), INFO = theme::rgb(0x1d3f57), OFFER = theme::rgb(0x2a4a2c), FOCUS = ui::FOCUS;
}

quick::Fix TDeckQuickPage::gpsFix() const {
  // Device GPS only with a fresh fix (15 s) and only after the user switched "Standort teilen" on.
  if (!api) return {};
  dev::Gps g = api->gps();
  if (!g.detected || !g.receiverOn || !g.fresh || !g.share) return {};
  return {true, g.lat, g.lon};
}

// Same recipients as the web chat/quick target list: channels, then chat contacts.
bool TDeckQuickPage::quickTarget(const dev::Api& a, unsigned index, char* id, size_t id_size, char* name, size_t name_size) {
  dev::Target t;
  for (unsigned i = 0, n = a.targetCount(); i < n; ++i) {
    if (!a.target(i, t) || (t.kind != dev::Kind::Channel && t.kind != dev::Kind::Chat)) continue;
    if (index--) continue;
    snprintf(id, id_size, "%s", t.id);
    snprintf(name, name_size, "%s %s", t.kind == dev::Kind::Channel ? "#" : "@", t.name);
    return true;
  }
  return false;
}

void TDeckQuickPage::setTarget(const char* id) {
  char tid[14], name[64];
  for (unsigned i = 0; api && i < 72 && quickTarget(*api, i, tid, sizeof(tid), name, sizeof(name)); ++i)
    if (!strcmp(tid, id)) { snprintf(target, sizeof(target), "%s", tid); snprintf(targetName, sizeof(targetName), "%s", name); targetIndex = i; return; }
}

void TDeckQuickPage::selectTarget(int direction) {
  unsigned count = 0;
  char id[14], name[64];
  while (api && count < 72 && quickTarget(*api, count, id, sizeof(id), name, sizeof(name))) ++count;
  if (!count) { snprintf(status, sizeof(status), "Noch keine Kanaele/Kontakte geladen"); return; }
  targetIndex = direction < 0 ? (targetIndex + count - 1) % count : (targetIndex + 1) % count;
  if (quickTarget(*api, targetIndex, id, sizeof(id), name, sizeof(name))) {
    // Snapshot the ID as well as the label. A later contact-list refresh must
    // never silently change the recipient selected by the user.
    snprintf(target, sizeof(target), "%s", id);
    snprintf(targetName, sizeof(targetName), "%s", name);
    draftChanged();
  }
}

bool TDeckQuickPage::refreshTargetName() {
  char id[14], name[64];
  for (unsigned i = 0; api && i < 72 && quickTarget(*api, i, id, sizeof(id), name, sizeof(name)); ++i) {
    if (strcmp(id, target)) continue;
    targetIndex = i;
    if (!strcmp(name, targetName)) return false;
    snprintf(targetName, sizeof(targetName), "%s", name); return true;
  }
  if (!strcmp(targetName, "[Ziel nicht verfuegbar]")) return false;
  snprintf(targetName, sizeof(targetName), "[Ziel nicht verfuegbar]"); return true;
}

void TDeckQuickPage::submit(bool position, uint32_t stamp) {
  if (submitted) {
    snprintf(status, sizeof(status), "Schon vorgemerkt: Text/Ort/Ziel aendern"); return;
  }
  char text[quick::TEXT_LIMIT + 1];
  quick::Result result = model.compose(position, gpsFix(), stamp, text, sizeof(text));
  switch (result) {
    case quick::Result::NoFix:
      snprintf(status, sizeof(status), "%s", api && !api->gps().share && model.place == 0
               ? "Standort teilen ist aus (Menue Standort) / Ort waehlen" : "Kein frischer GPS-Fix. Warten / Ort waehlen."); return;
    case quick::Result::InvalidTime:
      snprintf(status, sizeof(status), "Uhr fehlt: GPS-Fix abwarten / App verbinden"); return;
    case quick::Result::Empty:
      snprintf(status, sizeof(status), "Zuerst Meldungstext eingeben"); return;
    case quick::Result::TooLong:
      snprintf(status, sizeof(status), "Zu lang: max. 100 Bytes inklusive Standort"); return;
    default: break;
  }
  submitted = api && api->send(target, text, status, sizeof(status));
}

bool TDeckQuickPage::activate(uint32_t stamp) {
  if (picker >= 0) {
    choose(pickerSel);
    return false;
  }
  switch (model.focus) {
    case quick::Presets: model.selectPreset(model.preset); model.focus = quick::Text; draftChanged(); break;
    case quick::Text: model.focus = quick::PlaceSelect; break;
    case quick::PlaceSelect: model.cyclePlace(1); placeChosen = true; draftChanged(); break;
    case quick::Target: selectTarget(1); break;
    case quick::Send: submit(false, stamp); break;
    case quick::Position: submit(true, stamp); break;
    case quick::Home: return true;
    case quick::Clear: model.text[0] = 0; model.focus = quick::Text; draftChanged(); break;
    default: break;
  }
  return false;
}

bool TDeckQuickPage::key(uint8_t key, uint32_t stamp) {
  // The original keyboard has no TAB/ESC keys. Its documented ALT+C sends 0x0C.
  if (picker >= 0) {
    if (key == 27 || key == 8) {
      closePicker();
      return false;
    }
    if (key == '\r' || key == '\n') {
      choose(pickerSel);
      return false;
    }
    return false;
  }
  if (key == 12 || key == '\t') { model.focus = quick::Focus((model.focus + 1) % quick::FOCUS_COUNT); return false; }
  if (key == 27) return true;
  if (key == '\r' || key == '\n') return activate(stamp);
  if (key == 21) { model.text[0] = 0; model.focus = quick::Text; draftChanged(); return false; } // optional Ctrl+U
  if (model.focus == quick::Text) {
    if (key == 8 || key == 127) { model.backspace(); draftChanged(); }
    else if (key >= 32 && key <= 126) {
      if (model.append(char(key))) draftChanged();
      else snprintf(status, sizeof(status), "Text voll: Backspace oder Leeren waehlen");
    }
    return false;
  }
  if (key == 8 || key == 127) return true; // Backspace outside text returns home
  if (model.focus == quick::Presets && key >= '1' && key <= '8') {
    model.selectPreset(key - '1'); model.focus = quick::Text; draftChanged(); return false;
  }
  // Space cycles selections only; transmission always needs Enter/trackball.
  if (key == ' ' && (model.focus == quick::PlaceSelect || model.focus == quick::Target)) return activate(stamp);
  if (key == '[' || key == ']' || key == '(' || key == ')') {
    int dir = key == '[' || key == '(' ? -1 : 1;
    if (model.focus == quick::Presets) {
      model.selectPreset((model.preset + quick::PRESET_COUNT + dir) % quick::PRESET_COUNT); draftChanged();
    } else if (model.focus == quick::PlaceSelect) { model.cyclePlace(dir); placeChosen = true; draftChanged(); }
    else if (model.focus == quick::Target) selectTarget(dir);
  }
  return false;
}

bool TDeckQuickPage::move(int dx, int dy) {
  if (picker >= 0) {
    int n = int(choiceCount()), dir = dy ? dy : dx;
    if (!dir || !n) return false;
    pickerSel += dir > 0 ? 1 : -1;
    if (pickerSel < 0) pickerSel = 0;
    if (pickerSel >= n) pickerSel = n - 1;
    if (pickerSel < pickerTop) pickerTop = pickerSel;
    if (pickerSel >= pickerTop + touchui::VISIBLE_ROWS) pickerTop = pickerSel - touchui::VISIBLE_ROWS + 1;
    return true;
  }
  if (dy) { model.focus = quick::Focus((model.focus + quick::FOCUS_COUNT + (dy > 0 ? 1 : -1)) % quick::FOCUS_COUNT); return true; }
  if (!dx) return false;
  int dir = dx < 0 ? -1 : 1;
  if (model.focus == quick::Presets) { model.selectPreset((model.preset + quick::PRESET_COUNT + dir) % quick::PRESET_COUNT); draftChanged(); return true; }
  if (model.focus == quick::PlaceSelect) { model.cyclePlace(dir); placeChosen = true; draftChanged(); return true; }
  if (model.focus == quick::Target) { selectTarget(dir); return true; }
  return false;
}

unsigned TDeckQuickPage::choiceCount() const {
  if (picker == 0) return quick::PRESET_COUNT;
  if (picker == 1) return quick::PLACE_COUNT + 2;
  unsigned count = 0;
  char id[14], name[64];
  while (api && count < 72 && quickTarget(*api, count, id, sizeof(id), name, sizeof(name)))
    ++count;
  return count;
}
void TDeckQuickPage::openPicker(int which) {
  picker = which;
  pickerSel = which == 0 ? model.preset : which == 1 ? model.place + 1 : int(targetIndex);
  pickerTop = pickerSel / touchui::VISIBLE_ROWS * touchui::VISIBLE_ROWS;
  pickerRows = 0;
}
void TDeckQuickPage::choose(int index) {
  if (picker == 0) {
    model.selectPreset(index);
    model.focus = quick::Text;
    draftChanged();
  } else if (picker == 1) {
    model.place = index - 1;
    placeChosen = true;
    model.focus = quick::PlaceSelect;
    draftChanged();
  } else {
    char id[14], name[64];
    if (api && quickTarget(*api, unsigned(index), id, sizeof(id), name, sizeof(name))) {
      setTarget(id);
      model.focus = quick::Target;
      draftChanged();
    }
  }
  picker = -1;
}
int TDeckQuickPage::touchTarget(int x, int y) const {
  if (picker >= 0) {
    for (int r = 0; r < pickerRows; ++r)
      if (touchui::row(r).contains(x, y)) return 200 + r;
    return -1;
  }
  for (int i = 0; i < 7; ++i)
    if (touchui::quickControl(i).contains(x, y)) return 210 + i;
  return -1;
}
bool TDeckQuickPage::swipe(int dy) {
  if (picker < 0) return false;
  int n = int(choiceCount()), rows = abs(dy) / touchui::ROW + 1;
  pickerTop += dy < 0 ? rows : -rows;
  if (pickerTop > n - touchui::VISIBLE_ROWS) pickerTop = n - touchui::VISIBLE_ROWS;
  if (pickerTop < 0) pickerTop = 0;
  pickerSel = pickerTop;
  pickerRows = 0;
  return true;
}
bool TDeckQuickPage::tap(int x, int y, uint32_t stamp) {
  int hit = touchTarget(x, y);
  if (hit < 0) return false;
  if (picker >= 0) {
    int row = hit - 200, index = pickerTop + row;
    if (picker == 2) {
      // Use the displayed recipient ID even if the contact list changed since render.
      char shown[14];
      snprintf(shown, sizeof(shown), "%s", pickerIds[row]);
      bool exists = false;
      char id[14], name[64];
      for (unsigned i = 0; api && quickTarget(*api, i, id, sizeof(id), name, sizeof(name)); ++i)
        if (!strcmp(id, shown)) {
          setTarget(shown);
          exists = true;
          break;
        }
      if (exists) {
        model.focus = quick::Target;
        draftChanged();
        picker = -1;
      } else
        snprintf(status, sizeof(status), "Ziel nicht mehr verfuegbar / neu waehlen");
    } else
      choose(index);
    return false;
  }
  switch (hit - 210) {
  case 0:
    openPicker(0);
    break;
  case 1:
    model.focus = quick::Text;
    break;
  case 2:
    model.focus = quick::Clear;
    activate(stamp);
    break;
  case 3:
    openPicker(1);
    break;
  case 4:
    openPicker(2);
    break;
  case 5:
    model.focus = quick::Send;
    activate(stamp);
    break;
  case 6:
    model.focus = quick::Position;
    activate(stamp);
    break;
  }
  return false;
}

void TDeckQuickPage::renderPicker(DisplayDriver *d) {
  const char *title = picker == 0 ? "Vorlage" : picker == 1 ? "Ort waehlen" : "Ziel waehlen";
  ui::statusLine(d, title, true, unreadChat + unreadBoard);
  int n = int(choiceCount());
  if (pickerTop >= n) pickerTop = n > 0 ? n - 1 : 0;
  pickerRows = 0;
  for (int r = 0; r < touchui::VISIBLE_ROWS && pickerTop + r < n; ++r) {
    int index = pickerTop + r;
    char label[96];
    pickerIds[r][0] = 0;
    if (picker == 0)
      snprintf(label, sizeof(label), "%d  %s", index + 1, quick::PRESETS[index].label);
    else if (picker == 1)
      snprintf(label, sizeof(label), "%s",
               index == 0   ? "Ohne Standort"
               : index == 1 ? "Mein GPS (nur mit Freigabe)"
                            : quick::PLACES[index - 2].name);
    else
      quickTarget(*api, unsigned(index), pickerIds[r], sizeof(pickerIds[r]), label, sizeof(label));
    auto rect = touchui::row(r);
    ui::button(d, rect.x, rect.y, rect.w, rect.h, "", pickerSel == index);
    ui::text(d, rect.x + 10, rect.y + 9, WHITE, label, 24, 2);
    ++pickerRows;
  }
  char note[64];
  snprintf(note, sizeof(note), "%d-%d / %d | Wischen, dann antippen", n ? pickerTop + 1 : 0,
           pickerTop + pickerRows, n);
  ui::text(d, 4, 222, GREY, n ? note : "Noch keine Ziele geladen");
  d->endFrame();
}

bool TDeckQuickPage::tick(uint32_t now) {
  bool changed = false;
  if (int32_t(now - nextGps) >= 0) {
    nextGps = now + 1000;
    bool fix = quick::validFix(gpsFix());
    long sats = api ? api->gps().sats : 0;
    changed = fix != lastFix || sats != lastSatellites;
    if (refreshTargetName()) changed = true;
    lastFix = fix; lastSatellites = sats;
  }
  if (submitted && int32_t(now - nextStatus) >= 0) {
    nextStatus = now + 250;
    char next[96]; snprintf(next, sizeof(next), "%s", api ? api->sendStatus() : "");
    if (strcmp(next, status)) { snprintf(status, sizeof(status), "%s", next); changed = true; }
  }
  return changed;
}

void TDeckQuickPage::displayText(DisplayDriver* d, int x, int y, uint16_t color, const char* text, int limit) {
  char out[128]; int n = 0;
  for (size_t i = 0; text[i] && n < limit && n < int(sizeof(out)) - 1; ++i) {
    uint8_t c = text[i];
    if (c < 128) out[n++] = c >= 32 ? c : ' ';
    else if (c == 0xC3 && text[i + 1]) {
      uint8_t next = text[++i];
      switch (next) {
        case 0xA4: out[n++] = char(0x84); break;
        case 0xB6: out[n++] = char(0x94); break;
        case 0xBC: out[n++] = char(0x81); break;
        case 0x84: out[n++] = char(0x8E); break;
        case 0x96: out[n++] = char(0x99); break;
        case 0x9C: out[n++] = char(0x9A); break;
        case 0x9F: out[n++] = char(0xE1); break;
        default: out[n++] = '?'; break;
      }
    } else if ((c & 0xC0) != 0x80) out[n++] = '?';
  }
  out[n] = 0; d->nativeText(x, y, 1, color, out);
}

void TDeckQuickPage::render(DisplayDriver* d) {
  d->startFrame(BG);
  if (picker >= 0) {
    renderPicker(d);
    return;
  }
  ui::statusLine(d, "Meldung", true, unreadChat + unreadBoard, model.focus == quick::Home);
  char line[128];
  auto button = [&](int i, const char *label, bool on, bool primary) {
    auto r = touchui::quickControl(i);
    ui::button(d, r.x, r.y, r.w, r.h, label, on, primary);
  };
  snprintf(line, sizeof(line), "Vorlage: %s  >", quick::PRESETS[model.preset].label);
  button(0, line, model.focus == quick::Presets, false);
  auto text = touchui::quickControl(1);
  ui::button(d, text.x, text.y, text.w, text.h, "", model.focus == quick::Text);
  // Two physical-keyboard lines, cut only at complete UTF-8 character boundaries.
  char cp[160];
  theme::toCp437(model.text, cp, sizeof(cp));
  int count = 0, total = theme::countLines(cp, 40);
  theme::wrap(cp, 40, [&](const char *part, size_t n) {
    if (count++ < total - 2) return;
    ui::raw(d, 8, 106 + (count - 1 - (total > 2 ? total - 2 : 0)) * 10, WHITE, part, n);
  });
  snprintf(line, sizeof(line), "%u/100", unsigned(strlen(model.text)));
  ui::rightText(d, 256, 130, GREY, line);
  button(2, "Leeren", model.focus == quick::Clear, false);
  const char *place = model.place < 0    ? "Ohne Standort"
                      : model.place == 0 ? "Mein GPS"
                                         : quick::PLACES[model.place - 1].name;
  auto placeRect = touchui::quickControl(3);
  button(3, "", model.focus == quick::PlaceSelect, false);
  snprintf(line, sizeof(line), "Ort: %s >", place);
  ui::text(d, placeRect.x + 8, placeRect.y + 12, WHITE, line, 20);
  ui::text(d,placeRect.x+placeRect.w-12,placeRect.y+12,WHITE,">");
  // UTF-8-safe truncation is performed by the shared text helper, not snprintf's byte precision.
  auto targetRect = touchui::quickControl(4);
  button(4, "", model.focus == quick::Target, false);
  snprintf(line,sizeof(line),"Ziel: %s",targetName);
  ui::text(d, targetRect.x + 8, targetRect.y + 12, WHITE, line, 20);
  ui::text(d,targetRect.x+targetRect.w-12,targetRect.y+12,WHITE,">");
  button(5, "Meldung senden", model.focus == quick::Send, true);
  button(6, "Position senden", model.focus == quick::Position, false);
  displayText(d, 4, 222, FOCUS, status);
  if (strlen(status) > 52) displayText(d, 4, 231, FOCUS, status + 52);
  d->endFrame();
}
#endif
