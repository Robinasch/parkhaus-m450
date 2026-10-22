// Tarif- und Ausfahrtsregeln (US-04, US-06).
// Reines C++ ohne Arduino-Abhängigkeit, damit es auch nativ getestet werden kann.
#pragma once
#include <stdint.h>

namespace tarif {

// Alle Beträge in Rappen, alle Zeiten in Minuten.
const int32_t GRATIS_MIN = 15;          // die ersten 15 Minuten sind gratis
const int32_t PREIS_PRO_STUNDE = 200;   // CHF 2.00 pro angefangene Stunde
const int32_t TAGESMAXIMUM = 2000;      // CHF 20.00 pro angefangene 24 h
const int32_t AUSFAHRTSFRIST_MIN = 15;  // Zeit zwischen Zahlung und Ausfahrt

inline int32_t angefangeneStunden(int32_t minuten) {
  return (minuten + 59) / 60;
}

// Gebühr für einen Zeitraum ohne Gratiszeit (Tagesmaximum pro 24 h).
inline int32_t gebuehrOhneGratis(int32_t minuten) {
  if (minuten <= 0) return 0;
  int32_t tage = minuten / 1440;
  int32_t rest = minuten % 1440;
  int32_t restGebuehr = angefangeneStunden(rest) * PREIS_PRO_STUNDE;
  if (restGebuehr > TAGESMAXIMUM) restGebuehr = TAGESMAXIMUM;
  return tage * TAGESMAXIMUM + restGebuehr;
}

// Parkgebühr für die gesamte Parkdauer (mit Gratiszeit).
inline int32_t parkgebuehr(int32_t parkdauerMin) {
  if (parkdauerMin <= GRATIS_MIN) return 0;
  return gebuehrOhneGratis(parkdauerMin);
}

// Offener Betrag:
// - nicht bezahlt: volle Parkgebühr
// - bezahlt, Frist läuft noch: 0
// - bezahlt, Frist abgelaufen: Nachzahlung für die Zeit seit der Zahlung
inline int32_t offenerBetrag(int32_t parkdauerMin, bool bezahlt, int32_t minSeitZahlung) {
  if (!bezahlt) return parkgebuehr(parkdauerMin);
  if (minSeitZahlung <= AUSFAHRTSFRIST_MIN) return 0;
  return gebuehrOhneGratis(minSeitZahlung);
}

}  // namespace tarif
