#pragma once
struct FakeWire {
  int sda = -1, scl = -1;
  bool started = false;
  bool setPins(int a, int b) { if (started) return false; sda=a; scl=b; return true; }
  void begin(int a=8, int b=9) { if (started) return; if (sda<0) {sda=a; scl=b;} started=true; }
};
extern FakeWire Wire;
