// Prüfung und Normalisierung von Kennzeichen (US-02).
// Gültig: 2 Buchstaben (Kantonskürzel) + Leerzeichen + 1 bis 6 Ziffern, z.B. "ZH 123456".
#pragma once
#include <string>

namespace kennzeichen {

// Entfernt Leerzeichen am Anfang/Ende und wandelt in Grossbuchstaben um.
inline std::string normalisieren(const std::string &roh) {
  size_t a = roh.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = roh.find_last_not_of(" \t\r\n");
  std::string s = roh.substr(a, b - a + 1);
  for (char &c : s) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
  }
  return s;
}

// Erwartet ein bereits normalisiertes Kennzeichen.
inline bool istGueltig(const std::string &kz) {
  if (kz.size() < 4 || kz.size() > 9) return false;
  if (kz[0] < 'A' || kz[0] > 'Z' || kz[1] < 'A' || kz[1] > 'Z') return false;
  if (kz[2] != ' ') return false;
  for (size_t i = 3; i < kz.size(); i++) {
    if (kz[i] < '0' || kz[i] > '9') return false;
  }
  return true;
}

}  // namespace kennzeichen
