#pragma once
#include <stdint.h>

// Consume signed CHANGE-edge counts, never the resting levels of motion pins.
class TDeckTrackball {
public:
  enum Action { NONE, LEFT, RIGHT, UP, DOWN, CLICK };
  Action update(uint32_t now, int dx, int dy, bool pressed) {
    if (pressed != candidate_) { candidate_ = pressed; changedAt_ = now; }
    if (candidate_ != stable_ && now - changedAt_ >= 30) {
      stable_ = candidate_;
      if (stable_) { x_ = y_ = 0; lastAction_ = now; return CLICK; }
    }
    // Ignore rolling while pressing, and discard pulses during the cooldown.
    if (pressed || now - lastAction_ < 350) { x_ = y_ = 0; return NONE; }
    if (now - lastPulse_ > 200) x_ = y_ = 0;
    if (dx || dy) lastPulse_ = now;
    x_ = clamp(x_ + dx); y_ = clamp(y_ + dy);
    const int ax = x_ < 0 ? -x_ : x_, ay = y_ < 0 ? -y_ : y_;
    if (ax < 4 && ay < 4) return NONE;
    Action action = ax >= ay ? (x_ < 0 ? LEFT : RIGHT) : (y_ < 0 ? UP : DOWN);
    x_ = y_ = 0;
    lastAction_ = now;
    return action;
  }
private:
  static int clamp(int n) { return n < -32 ? -32 : (n > 32 ? 32 : n); }
  int x_ = 0, y_ = 0;
  uint32_t lastAction_ = 0, lastPulse_ = 0, changedAt_ = 0;
  bool candidate_ = false, stable_ = false;
};
