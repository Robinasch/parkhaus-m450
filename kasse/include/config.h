// Konfiguration Pico 1 (Kasse). Muss zu schranken/include/config.h passen.
#pragma once

// Pico 1 öffnet ein eigenes WLAN (Access Point). Pico 2 und der Laptop verbinden sich damit.
#define WLAN_SSID "Parkhaus-M450"
#define WLAN_PASSWORT "parkhaus123"

#define KASSE_IP 192, 168, 42, 1
#define NETZMASKE 255, 255, 255, 0

#define KAPAZITAET_STANDARD 50

// Debug-Seite /debug zum Verstellen von Zeiten (nur für Tests, siehe Testkonzept).
// Für den produktiven Betrieb auf 0 setzen.
#define DEBUG_SEITE 1
