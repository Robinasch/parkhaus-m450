// Pico 2 – Schranken
// - verbindet sich mit dem WLAN der Kasse (feste IP 192.168.42.10)
// - Weboberfläche simuliert die Kameras: Kennzeichen bei Ein-/Ausfahrt eingeben
// - meldet Einfahrten an die Kasse und fragt bei der Ausfahrt den Bezahlstatus ab
// - steuert die beiden Schranken (Servos SG90)
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Servo.h>
#include <string>

#include "config.h"
#include "kennzeichen.h"
#include "webseite.h"

struct Schranke {
  const char *name;
  int pin;
  Servo servo;
  bool offen = false;
  unsigned long geoeffnetUm = 0;

  void begin() {
    servo.attach(pin);
    servo.write(WINKEL_ZU);
  }
  void oeffnen() {
    servo.write(WINKEL_OFFEN);
    offen = true;
    geoeffnetUm = millis();
    Serial.printf("[Schranke %s] offen\n", name);
  }
  // Schliesst die Schranke automatisch nach SCHRANKE_OFFEN_MS (US-01 AK2)
  void update() {
    if (offen && millis() - geoeffnetUm >= SCHRANKE_OFFEN_MS) {
      servo.write(WINKEL_ZU);
      offen = false;
      Serial.printf("[Schranke %s] zu\n", name);
    }
  }
};

Schranke einfahrt{"Einfahrt", PIN_SCHRANKE_EINFAHRT};
Schranke ausfahrt{"Ausfahrt", PIN_SCHRANKE_AUSFAHRT};
WebServer server(80);
bool kasseErreichbar = false;
unsigned long letzterWlanVersuch = 0;

String urlEncode(const String &s) {
  String out;
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum((unsigned char)c)) {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}

// Sendet eine Anfrage an die Kasse. Rückgabe: HTTP-Status oder <= 0 bei Verbindungsfehler.
int anKasse(const char *pfad, const String &kz, String &antwort) {
  if (WiFi.status() != WL_CONNECTED) {
    kasseErreichbar = false;
    return -1;
  }
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(KASSE_TIMEOUT_MS);
  http.begin(client, String(KASSE_URL) + pfad);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = http.POST("kz=" + urlEncode(kz));
  if (code > 0) antwort = http.getString();
  http.end();
  kasseErreichbar = code > 0;
  return code;
}

void antworten(bool ok, const String &meldung) {
  String m = meldung;
  m.replace("\"", "'");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8",
              "{\"ok\":" + String(ok ? "true" : "false") + ",\"meldung\":\"" + m + "\"}");
}

// Gemeinsamer Ablauf für Ein- und Ausfahrt
void kameraEreignis(const char *pfad, Schranke &schranke) {
  std::string s = kennzeichen::normalisieren(server.arg("kz").c_str());
  String kz(s.c_str());
  if (!kennzeichen::istGueltig(s)) {  // US-02: ungültige Kennzeichen gar nicht erst senden
    antworten(false, "Ungültiges Kennzeichen");
    return;
  }
  String antwort;
  int code = anKasse(pfad, kz, antwort);
  if (code <= 0) {  // US-07: ohne Kasse bleibt die Schranke zu
    antworten(false, "Kasse nicht erreichbar – Schranke bleibt geschlossen");
    return;
  }
  if (code == 200) schranke.oeffnen();
  antworten(code == 200, antwort);
}

void handleStatus() {
  String j = "{\"einfahrt\":" + String(einfahrt.offen ? "true" : "false");
  j += ",\"ausfahrt\":" + String(ausfahrt.offen ? "true" : "false");
  j += ",\"wlan\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  j += ",\"kasse\":" + String(kasseErreichbar ? "true" : "false") + "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

void wlanVerbinden() {
  Serial.printf("Verbinde mit WLAN %s ...\n", WLAN_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(SCHRANKEN_IP), IPAddress(KASSE_IP), IPAddress(KASSE_IP), IPAddress(NETZMASKE));
  WiFi.begin(WLAN_SSID, WLAN_PASSWORT);
  letzterWlanVersuch = millis();
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("Pico 2 – Schranken startet");

  einfahrt.begin();
  ausfahrt.begin();

  wlanVerbinden();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Verbunden. Schranken: http://%s/\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WLAN nicht erreichbar – neuer Versuch im Hintergrund");
  }

  server.on("/", HTTP_GET, [] { server.send(200, "text/html; charset=utf-8", SEITE_SCHRANKEN); });
  server.on("/api/einfahrt", HTTP_POST, [] { kameraEreignis("/api/einfahrt", einfahrt); });
  server.on("/api/ausfahrt", HTTP_POST, [] { kameraEreignis("/api/ausfahrt", ausfahrt); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.onNotFound([] { server.send(404, "text/plain", "Nicht gefunden"); });
  server.begin();

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  server.handleClient();
  einfahrt.update();
  ausfahrt.update();

  // US-07 AK3: nach einem Unterbruch automatisch wieder verbinden
  if (WiFi.status() != WL_CONNECTED && millis() - letzterWlanVersuch > 10000) {
    wlanVerbinden();
  }
  // Onboard-LED: an = mit WLAN verbunden
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
}
