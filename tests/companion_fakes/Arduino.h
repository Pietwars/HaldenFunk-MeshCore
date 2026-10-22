#pragma once
// Host fakes for LocalCompanion.h (Issue #35): just enough Arduino for the companion client and its files.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <string>
extern uint32_t fake_millis;
inline uint32_t millis() { return fake_millis; }
class String {
  std::string s;
public:
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const std::string &v) : s(v) {}
  size_t length() const { return s.size(); }
  char operator[](size_t i) const { return i < s.size() ? s[i] : 0; }
  String substring(size_t a, size_t b) const { if (a > s.size()) return String(); if (b > s.size()) b = s.size(); return b > a ? String(s.substr(a, b - a)) : String(); }
  String substring(size_t a) const { return substring(a, s.size()); }
  bool operator==(const String &o) const { return s == o.s; }
  bool operator!=(const String &o) const { return s != o.s; }
  bool operator==(const char *o) const { return s == o; }
  bool operator!=(const char *o) const { return s != o; }
  String operator+(const String &o) const { return String(s + o.s); }
  String &operator+=(char c) { s += c; return *this; }
  String &operator+=(const String &o) { s += o.s; return *this; }
  const char *c_str() const { return s.c_str(); }
  void reserve(size_t n) { s.reserve(n); }
  void toLowerCase() { for (auto &c : s) c = char(tolower((unsigned char)c)); }
};
struct FakeSerial {
  void printf(const char *f, ...) { va_list a; va_start(a, f); vfprintf(stderr, f, a); va_end(a); }
  void println(const char *t) { fprintf(stderr, "%s\n", t); }
};
extern FakeSerial Serial;
