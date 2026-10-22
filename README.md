# Parkhaus mit Schranken – M450 INA24-B

Umsetzung des Parksystems (Auftrag A01-03) mit zwei Raspberry Pi Pico W und dem Arduino-Framework.
Grundlage sind die User Stories aus dem Dokument *Anforderungen Parksystem* (Auftrag A01-01-02).

## Gruppenmitglieder

| Name | Beitrag |
|---|---|
| Robin Aschwanden | Anforderungen, Testkonzept, Umsetzung Kasse und Schranken |

## Kurze Beschreibung der Umsetzung

Das System besteht aus zwei Pico W, die über WLAN miteinander kommunizieren:

```
                       WLAN "Parkhaus-M450"
 Laptop / Handy  ───────────────┬──────────────────────────┐
                                │                          │
                  ┌─────────────▼────────────┐   ┌─────────▼──────────────┐
                  │ Pico 2 – Schranken       │   │ Pico 1 – Kasse         │
                  │ http://192.168.42.10     │   │ http://192.168.42.1    │
                  │ Kamera-Simulation (Web)  │──▶│ Access Point (WLAN)    │
                  │ Servo Einfahrt (GP8)     │   │ Fahrzeugliste, Tarif   │
                  │ Servo Ausfahrt (GP9)     │◀──│ Bezahlen, Übersicht    │
                  └──────────────────────────┘   └────────────────────────┘
```

- **Pico 1 – Kasse** (`kasse/`)
  öffnet das WLAN `Parkhaus-M450` und speichert alle parkierten Fahrzeuge mit Einfahrtszeit. Er berechnet die Parkgebühr und stellt die Weboberfläche für Kasse und Automat bereit (Suchen, Bezahlen, Übersicht). Zudem beantwortet er die Anfragen von Pico 2.
- **Pico 2 – Schranken** (`schranken/`)
  verbindet sich mit dem WLAN der Kasse. Seine Weboberfläche ersetzt die Kameras: Das Kennzeichen wird bei der Ein- oder Ausfahrt von Hand eingegeben. Pico 2 meldet die Einfahrt an die Kasse und fragt bei der Ausfahrt nach, ob bezahlt wurde. Fällt die Antwort positiv aus, öffnet er die passende Schranke (Servo SG90) für 5 Sekunden.

Fachliche Regeln (wie in den Anforderungen festgelegt):

| Regel | Festlegung |
|---|---|
| Kennzeichen | 2 Buchstaben + Leerzeichen + 1–6 Ziffern, z.B. `ZH 123456`. Kleinbuchstaben werden umgewandelt. |
| Tarif | erste 15 Min. gratis, danach CHF 2.00 pro angefangene Stunde, max. CHF 20.00 pro angefangene 24 h |
| Ausfahrtsfrist | 15 Min. nach der Zahlung; danach Nachzahlung für die Zeit seit der Zahlung |
| Kapazität | 50 Plätze |

## Umgesetzte User Stories

| ID | User Story | Status | Umsetzung |
|---|---|---|---|
| US-01 | Automatische Einfahrt | ✅ umgesetzt | Pico 2 öffnet die Einfahrtsschranke, sobald die Kasse die Einfahrt bestätigt. Nach 5 s schliesst sie. Bei vollem Parkhaus erscheint „Parkhaus besetzt“. |
| US-02 | Ungültige Kennzeichen abweisen | ✅ umgesetzt | Das Format wird auf beiden Picos geprüft (`kennzeichen.h`). Doppelte Einfahrten werden abgewiesen. |
| US-03 | Einfahrtszeit speichern | ✅ umgesetzt | Die Kasse speichert die Einfahrtszeit pro Fahrzeug. Die Uhrzeit kommt beim Öffnen der Weboberfläche vom Browser. |
| US-04 | Parkgebühr berechnen | ✅ umgesetzt | `kasse/include/tarif.h`, dazu automatische Unit-Tests |
| US-05 | Parkgebühr bezahlen | ✅ umgesetzt | Kasse/Automat: Kennzeichen suchen, Betrag anzeigen, „Bezahlen“. Eine doppelte Zahlung wird verhindert. |
| US-06 | Ausfahrt nur nach Zahlung | ✅ umgesetzt | Pico 2 fragt die Kasse. Die Schranke öffnet nur bei offenem Betrag CHF 0.00 (bezahlt innerhalb der Frist oder Gratiszeit). |
| US-07 | Sicheres Verhalten bei Störung | ✅ umgesetzt | Ist die Kasse nicht erreichbar, bleibt die Schranke zu (Timeout 3 s). Danach verbindet sich Pico 2 automatisch wieder mit dem WLAN. |
| US-08 | Übersicht parkierte Fahrzeuge | ✅ umgesetzt | Liste auf der Kasse mit Kennzeichen, Einfahrt, Dauer, offenem Betrag und Status. Freie Plätze werden angezeigt, die neueste Einfahrt steht zuoberst. |

Nicht umgesetzt, wie in den Anforderungen festgelegt: echte Kameras, echte Zahlungsmittel und das Speichern der Daten über einen Neustart hinaus.

## Hardware und Verdrahtung

- 2× Raspberry Pi Pico W
- 2× Servo SG90 an **Pico 2**

| Servo-Kabel | Pico 2 |
|---|---|
| rot (VCC) | VBUS (Pin 40) |
| braun (GND) | GND (z.B. Pin 8 oder 13) |
| orange (Signal) Einfahrt | GP8 (Pin 11) |
| orange (Signal) Ausfahrt | GP9 (Pin 12) |

Pico 1 braucht nur USB-Strom. Die Onboard-LED zeigt den Zustand an:
- Pico 1: LED an = Kasse bereit
- Pico 2: LED an = mit dem WLAN verbunden

## Inbetriebnahme

Voraussetzung ist VS Code mit PlatformIO (siehe A00 Entwicklungsumgebung).

1. Den Ordner `kasse/` in VS Code öffnen und **Upload** ausführen (Pico 1).
2. Den Ordner `schranken/` öffnen und **Upload** ausführen (Pico 2).
   Klappt der Upload nicht: Mit **Build** erzeugen, Pico mit gedrückter BOOTSEL-Taste einstecken und `.pio/build/pico/firmware.uf2` auf das Laufwerk `RPI-RP2` kopieren.
3. **Zuerst Pico 1** mit Strom versorgen, danach Pico 2.
4. Laptop oder Handy mit dem WLAN **`Parkhaus-M450`** verbinden (Passwort `parkhaus123`).
5. Im Browser öffnen:
   - Kamera-Simulation und Schranken: <http://192.168.42.10>
   - Kasse / Automat: <http://192.168.42.1>
   - Debug-Seite (nur Test): <http://192.168.42.1/debug>

WLAN-Name, Passwort, IP-Adressen und Pins stehen in `kasse/include/config.h` und `schranken/include/config.h`. Die Werte müssen in beiden Dateien übereinstimmen.

## Bedienung

1. **Einfahrt:** Auf der Schranken-Seite das Kennzeichen im Feld *Einfahrt* eingeben und auf **Einfahren** klicken. Die Schranke öffnet sich.
2. **Bezahlen:** Auf der Kassen-Seite das Kennzeichen suchen oder in der Liste anklicken. Parkdauer und Betrag werden angezeigt. Dann auf **Bezahlen** klicken.
3. **Ausfahrt:** Auf der Schranken-Seite das Kennzeichen im Feld *Ausfahrt* eingeben und auf **Ausfahren** klicken. Ist bezahlt, öffnet sich die Schranke. Sonst erscheint ein Hinweis mit dem offenen Betrag.

Die **Debug-Seite** ist für die Tests aus dem Testkonzept gedacht. Dort kann man Einfahrts- und Zahlungszeiten zurückstellen, damit Gebührentests ohne Warten möglich sind. Ausserdem lassen sich die Kapazität ändern und alle Daten löschen. Für den produktiven Betrieb wird sie in `kasse/include/config.h` mit `DEBUG_SEITE 0` abgeschaltet.

## Tests

- **Manuelle Tests:** siehe Testkonzept (Auftrag A01-10), Testfälle TC-01 bis TC-23
- **Automatische Unit-Tests** für Tarif und Kennzeichenprüfung (laufen auf dem PC, ohne Pico):

  ```bash
  cd kasse
  pio test -e native
  ```

## Schnittstelle zwischen den Picos

Pico 2 sendet `POST` mit `kz=<Kennzeichen>` an die Kasse. Die Antwort ist Text mit HTTP-Statuscode:

| Endpunkt | Status | Bedeutung |
|---|---|---|
| `/api/einfahrt` | 200 | Einfahrt registriert, Schranke öffnen |
| | 400 / 409 / 503 | ungültig / bereits im Parkhaus / Parkhaus besetzt |
| `/api/ausfahrt` | 200 | bezahlt oder gratis, Schranke öffnen, Fahrzeug wird entfernt |
| | 402 / 403 / 404 | nicht bezahlt / Ausfahrtsfrist überschritten / nicht registriert |

## Projektstruktur

```
kasse/                      Pico 1
  include/config.h          WLAN, IP, Kapazität, Debug-Schalter
  include/tarif.h           Gebührenberechnung (US-04, US-06)
  include/kennzeichen.h     Kennzeichenprüfung (US-02)
  include/webseiten.h       HTML Kasse und Debug-Seite
  src/main.cpp              Webserver, Fahrzeugliste, API
  test/test_tarif/          Unit-Tests (pio test -e native)
schranken/                  Pico 2
  include/config.h          WLAN, IP, Servo-Pins
  include/kennzeichen.h     Kennzeichenprüfung (Kopie)
  include/webseite.h        HTML Kamera-Simulation
  src/main.cpp              Webserver, Schranken, Anfragen an die Kasse
```

## Bekannte Einschränkungen

- Die Daten liegen nur im RAM. Nach einem Neustart von Pico 1 sind alle Fahrzeuge weg.
- Der Pico hat keine Echtzeituhr. Datum und Uhrzeit übernimmt die Kasse vom Browser, sobald eine der Weboberflächen geöffnet wird.
- Die interne Uhr basiert auf `millis()` und läuft nach rund 49 Tagen über.
