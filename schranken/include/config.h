// Konfiguration Pico 2 (Schranken). Muss zu kasse/include/config.h passen.
#pragma once

// WLAN der Kasse (Pico 1 ist Access Point)
#define WLAN_SSID "Parkhaus-M450"
#define WLAN_PASSWORT "parkhaus123"

#define SCHRANKEN_IP 192, 168, 42, 10
#define KASSE_IP 192, 168, 42, 1
#define NETZMASKE 255, 255, 255, 0
#define KASSE_URL "http://192.168.42.1"

// Servos SG90: rot -> VBUS (Pin 40), braun -> GND, orange -> Signal
#define PIN_SCHRANKE_EINFAHRT 8  // GP8 (Pin 11)
#define PIN_SCHRANKE_AUSFAHRT 9  // GP9 (Pin 12)

#define WINKEL_ZU 0
#define WINKEL_OFFEN 90
#define SCHRANKE_OFFEN_MS 5000  // Schranke schliesst nach 5 Sekunden

#define KASSE_TIMEOUT_MS 3000  // US-07: Meldung "Kasse nicht erreichbar" innert 5 s
