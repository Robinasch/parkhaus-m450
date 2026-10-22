// Unit-Tests für Tarif und Kennzeichen. Ausführen im Ordner kasse: pio test -e native
// Die Testwerte entsprechen den Grenzwert-Testfällen aus dem Testkonzept (TC-04, TC-06 bis TC-10, TC-17).
#include <unity.h>
#include "../../include/kennzeichen.h"
#include "../../include/tarif.h"

void setUp() {}
void tearDown() {}

void test_gratiszeit() {
  TEST_ASSERT_EQUAL(0, tarif::parkgebuehr(0));
  TEST_ASSERT_EQUAL(0, tarif::parkgebuehr(15));   // TC-06
  TEST_ASSERT_EQUAL(200, tarif::parkgebuehr(16)); // TC-07
}

void test_angefangene_stunden() {
  TEST_ASSERT_EQUAL(200, tarif::parkgebuehr(60));  // TC-08
  TEST_ASSERT_EQUAL(400, tarif::parkgebuehr(61));  // TC-08
  TEST_ASSERT_EQUAL(400, tarif::parkgebuehr(90));  // TC-11
  TEST_ASSERT_EQUAL(600, tarif::parkgebuehr(125)); // TC-21
  TEST_ASSERT_EQUAL(800, tarif::parkgebuehr(200)); // TC-09
}

void test_tagesmaximum() {
  TEST_ASSERT_EQUAL(2000, tarif::parkgebuehr(12 * 60));  // TC-10
  TEST_ASSERT_EQUAL(2000, tarif::parkgebuehr(24 * 60));
  TEST_ASSERT_EQUAL(2200, tarif::parkgebuehr(24 * 60 + 1));
  TEST_ASSERT_EQUAL(4000, tarif::parkgebuehr(48 * 60));
}

void test_offener_betrag() {
  TEST_ASSERT_EQUAL(400, tarif::offenerBetrag(90, false, 0));
  TEST_ASSERT_EQUAL(0, tarif::offenerBetrag(90, true, 10));    // Frist läuft
  TEST_ASSERT_EQUAL(0, tarif::offenerBetrag(90, true, 15));    // genau 15 Min.
  TEST_ASSERT_EQUAL(200, tarif::offenerBetrag(106, true, 16)); // TC-17 Nachzahlung
  TEST_ASSERT_EQUAL(0, tarif::offenerBetrag(10, false, 0));    // TC-16 Gratiszeit
}

void test_kennzeichen_gueltig() {
  TEST_ASSERT_TRUE(kennzeichen::istGueltig("ZH 123456"));
  TEST_ASSERT_TRUE(kennzeichen::istGueltig("LU 7"));
  TEST_ASSERT_EQUAL_STRING("ZH 111", kennzeichen::normalisieren("  zh 111 ").c_str());  // TC-02
}

void test_kennzeichen_ungueltig() {  // TC-03, TC-04
  const char *ungueltig[] = {"", "123456", "Z 123", "ZH-123", "ZH ABC", "ZHH 12", "ZH 1234567", "ZH123", "ZH  12"};
  for (const char *kz : ungueltig) {
    TEST_ASSERT_FALSE_MESSAGE(kennzeichen::istGueltig(kennzeichen::normalisieren(kz)), kz);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_gratiszeit);
  RUN_TEST(test_angefangene_stunden);
  RUN_TEST(test_tagesmaximum);
  RUN_TEST(test_offener_betrag);
  RUN_TEST(test_kennzeichen_gueltig);
  RUN_TEST(test_kennzeichen_ungueltig);
  return UNITY_END();
}
