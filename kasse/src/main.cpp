// Pico 1 – Kasse
// - öffnet das WLAN "Parkhaus-M450" (Access Point, 192.168.42.1)
// - speichert alle parkierten Fahrzeuge mit Einfahrtszeit (US-03)
// - berechnet die Parkgebühr (US-04) und nimmt Zahlungen entgegen (US-05)
// - beantwortet Ein-/Ausfahrtsanfragen von Pico 2 (US-01, US-02, US-06)
// - zeigt eine Übersicht der parkierten Fahrzeuge (US-08)
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <string>

#include "config.h"
#include "kennzeichen.h"
#include "tarif.h"
#include "webseiten.h"

struct Fahrzeug {
  bool belegt = false;
  String kz;
  int64_t einfahrt = 0;  // Sekunden (interne Uhr, siehe jetzt())
  bool bezahlt = false;
  int64_t zahlung = 0;
  int32_t bezahltTotal = 0;  // Rappen
};

const int MAX_FAHRZEUGE = 100;
Fahrzeug fahrzeuge[MAX_FAHRZEUGE];
int kapazitaet = KAPAZITAET_STANDARD;

// Interne Uhr in Sekunden seit dem Start. Die Weboberflächen senden beim Laden die
// aktuelle Uhrzeit des Browsers, damit Datum/Uhrzeit korrekt angezeigt werden können.
int64_t zeitBasis = 0;  // Unix-Zeit beim Start des Picos
bool uhrGestellt = false;

WebServer server(80);

int64_t jetzt() { return (int64_t)(millis() / 1000); }
int64_t unixZeit(int64_t intern) { return zeitBasis + intern; }

int32_t minutenSeit(int64_t zeitpunkt) {
  int64_t d = jetzt() - zeitpunkt;
  if (d < 0) d = 0;
  return (int32_t)(d / 60);
}

int anzahlParkiert() {
  int n = 0;
  for (auto &f : fahrzeuge) n += f.belegt ? 1 : 0;
  return n;
}

Fahrzeug *finden(const String &kz) {
  for (auto &f : fahrzeuge) {
    if (f.belegt && f.kz == kz) return &f;
  }
  return nullptr;
}

int32_t offen(const Fahrzeug &f) {
  return tarif::offenerBetrag(minutenSeit(f.einfahrt), f.bezahlt, minutenSeit(f.zahlung));
}

String chf(int32_t rappen) {
  char buf[24];
  snprintf(buf, sizeof(buf), "CHF %ld.%02ld", (long)(rappen / 100), (long)(rappen % 100));
  return String(buf);
}

// Liest das Argument "kz", normalisiert es und prüft das Format.
bool kennzeichenLesen(String &kz) {
  std::string s = kennzeichen::normalisieren(server.arg("kz").c_str());
  kz = String(s.c_str());
  return kennzeichen::istGueltig(s);
}

void sendeText(int code, const String &text) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "text/plain; charset=utf-8", text);
}

void sendeJson(int code, const String &json) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", json);
}

String zahl(int64_t v) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%lld", (long long)v);
  return String(buf);
}

String fahrzeugJson(const Fahrzeug &f) {
  String j = "{\"kz\":\"" + f.kz + "\"";
  j += ",\"einfahrt\":" + zahl(unixZeit(f.einfahrt));
  j += ",\"dauerMin\":" + String(minutenSeit(f.einfahrt));
  j += ",\"gebuehr\":" + String(tarif::parkgebuehr(minutenSeit(f.einfahrt)));
  j += ",\"offen\":" + String(offen(f));
  j += ",\"bezahlt\":" + String(f.bezahlt ? "true" : "false");
  j += ",\"zahlung\":" + zahl(unixZeit(f.zahlung));
  j += ",\"bezahltTotal\":" + String(f.bezahltTotal);
  j += "}";
  return j;
}

// ---------------------------------------------------------------- Einfahrt (US-01, US-02, US-03)
void handleEinfahrt() {
  String kz;
  if (!kennzeichenLesen(kz)) {
    sendeText(400, "Ungültiges Kennzeichen");
    return;
  }
  if (finden(kz)) {
    sendeText(409, "Fahrzeug ist bereits im Parkhaus");
    return;
  }
  if (anzahlParkiert() >= kapazitaet) {
    sendeText(503, "Parkhaus besetzt");
    return;
  }
  for (auto &f : fahrzeuge) {
    if (!f.belegt) {
      f = Fahrzeug();
      f.belegt = true;
      f.kz = kz;
      f.einfahrt = jetzt();
      Serial.printf("[Einfahrt] %s\n", kz.c_str());
      sendeText(200, "Willkommen " + kz);
      return;
    }
  }
  sendeText(503, "Parkhaus besetzt");
}

// ---------------------------------------------------------------- Ausfahrt (US-06)
void handleAusfahrt() {
  String kz;
  if (!kennzeichenLesen(kz)) {
    sendeText(400, "Ungültiges Kennzeichen");
    return;
  }
  Fahrzeug *f = finden(kz);
  if (!f) {
    sendeText(404, "Fahrzeug nicht registriert – bitte Personal rufen");
    return;
  }
  int32_t betrag = offen(*f);
  if (betrag == 0) {
    Serial.printf("[Ausfahrt] %s\n", kz.c_str());
    f->belegt = false;
    sendeText(200, "Gute Fahrt");
  } else if (f->bezahlt) {
    sendeText(403, "Ausfahrtszeit überschritten – Nachzahlung " + chf(betrag) + " an der Kasse");
  } else {
    sendeText(402, "Bitte zuerst an der Kasse bezahlen (offen: " + chf(betrag) + ")");
  }
}

// ---------------------------------------------------------------- Kasse (US-04, US-05, US-08)
void handleFahrzeug() {
  String kz;
  if (!kennzeichenLesen(kz)) {
    sendeJson(400, "{\"fehler\":\"Ungültiges Kennzeichen\"}");
    return;
  }
  Fahrzeug *f = finden(kz);
  if (!f) {
    sendeJson(404, "{\"fehler\":\"Fahrzeug nicht gefunden\"}");
    return;
  }
  sendeJson(200, fahrzeugJson(*f));
}

void handleBezahlen() {
  String kz;
  if (!kennzeichenLesen(kz)) {
    sendeJson(400, "{\"fehler\":\"Ungültiges Kennzeichen\"}");
    return;
  }
  Fahrzeug *f = finden(kz);
  if (!f) {
    sendeJson(404, "{\"fehler\":\"Fahrzeug nicht gefunden\"}");
    return;
  }
  int32_t betrag = offen(*f);
  if (betrag == 0) {
    sendeJson(409, f->bezahlt ? "{\"fehler\":\"Bereits bezahlt\"}"
                              : "{\"fehler\":\"Nichts zu bezahlen (Gratiszeit)\"}");
    return;
  }
  f->bezahlt = true;
  f->zahlung = jetzt();
  f->bezahltTotal += betrag;
  Serial.printf("[Zahlung] %s %s\n", kz.c_str(), chf(betrag).c_str());
  sendeJson(200, fahrzeugJson(*f));
}

void handleListe() {
  // Neueste Einfahrt zuoberst (US-08 AK3)
  int idx[MAX_FAHRZEUGE];
  int n = 0;
  for (int i = 0; i < MAX_FAHRZEUGE; i++) {
    if (fahrzeuge[i].belegt) idx[n++] = i;
  }
  for (int i = 1; i < n; i++) {
    for (int k = i; k > 0 && fahrzeuge[idx[k]].einfahrt > fahrzeuge[idx[k - 1]].einfahrt; k--) {
      int t = idx[k];
      idx[k] = idx[k - 1];
      idx[k - 1] = t;
    }
  }
  String j = "{\"kapazitaet\":" + String(kapazitaet);
  j += ",\"frei\":" + String(max(0, kapazitaet - n));
  j += ",\"uhr\":" + String(uhrGestellt ? "true" : "false");
  j += ",\"debug\":" + String(DEBUG_SEITE ? "true" : "false");
  j += ",\"fahrzeuge\":[";
  for (int i = 0; i < n; i++) {
    if (i) j += ",";
    j += fahrzeugJson(fahrzeuge[idx[i]]);
  }
  j += "]}";
  sendeJson(200, j);
}

// Browser sendet seine Uhrzeit (Unix-Sekunden), damit Datum/Uhrzeit stimmen.
void handleZeit() {
  long long t = atoll(server.arg("t").c_str());
  if (t > 1600000000LL) {
    zeitBasis = t - jetzt();
    uhrGestellt = true;
  }
  sendeText(200, "ok");
}

#if DEBUG_SEITE
// ---------------------------------------------------------------- Debug (nur Test-Build)
void handleDebugEinfahrt() {
  String kz;
  kennzeichenLesen(kz);
  Fahrzeug *f = finden(kz);
  if (!f) {
    sendeText(404, "Fahrzeug nicht gefunden");
    return;
  }
  f->einfahrt = jetzt() - (int64_t)server.arg("min").toInt() * 60;
  sendeText(200, "Einfahrtszeit gesetzt");
}

void handleDebugZahlung() {
  String kz;
  kennzeichenLesen(kz);
  Fahrzeug *f = finden(kz);
  if (!f || !f->bezahlt) {
    sendeText(404, "Fahrzeug nicht gefunden oder nicht bezahlt");
    return;
  }
  f->zahlung = jetzt() - (int64_t)server.arg("min").toInt() * 60;
  sendeText(200, "Zahlungszeit gesetzt");
}

void handleDebugKapazitaet() {
  int k = server.arg("n").toInt();
  if (k < 1 || k > MAX_FAHRZEUGE) {
    sendeText(400, "Kapazität muss zwischen 1 und " + String(MAX_FAHRZEUGE) + " liegen");
    return;
  }
  kapazitaet = k;
  sendeText(200, "Kapazität gesetzt");
}

void handleDebugReset() {
  for (auto &f : fahrzeuge) f = Fahrzeug();
  kapazitaet = KAPAZITAET_STANDARD;
  sendeText(200, "Alle Daten gelöscht");
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("Pico 1 – Kasse startet");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(KASSE_IP), IPAddress(KASSE_IP), IPAddress(NETZMASKE));
  WiFi.softAP(WLAN_SSID, WLAN_PASSWORT);
  Serial.printf("WLAN: %s  Kasse: http://%s/\n", WLAN_SSID, WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, [] { server.send(200, "text/html; charset=utf-8", SEITE_KASSE); });
  server.on("/api/einfahrt", HTTP_POST, handleEinfahrt);
  server.on("/api/ausfahrt", HTTP_POST, handleAusfahrt);
  server.on("/api/fahrzeug", HTTP_GET, handleFahrzeug);
  server.on("/api/fahrzeuge", HTTP_GET, handleListe);
  server.on("/api/bezahlen", HTTP_POST, handleBezahlen);
  server.on("/api/zeit", HTTP_POST, handleZeit);
#if DEBUG_SEITE
  server.on("/debug", HTTP_GET, [] { server.send(200, "text/html; charset=utf-8", SEITE_DEBUG); });
  server.on("/api/debug/einfahrt", HTTP_POST, handleDebugEinfahrt);
  server.on("/api/debug/zahlung", HTTP_POST, handleDebugZahlung);
  server.on("/api/debug/kapazitaet", HTTP_POST, handleDebugKapazitaet);
  server.on("/api/debug/reset", HTTP_POST, handleDebugReset);
#endif
  server.onNotFound([] { sendeText(404, "Nicht gefunden"); });
  server.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED an = Kasse bereit
}

void loop() {
  server.handleClient();
}
