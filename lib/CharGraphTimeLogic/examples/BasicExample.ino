/**
 * CharGraph Time Logic - Basic Example
 *
 * Simple example showing how to use the library:
 * - Convert time to German words
 * - Display LED hex value
 *
 * Tested on: WEMOS D1 Mini (ESP8266)
 *
 * Hardware:
 * - ESP8266 / WEMOS D1 Mini
 * - USB Serial connection
 *
 * Instructions:
 * 1. Install library in Arduino/libraries/CharGraphTimeLogic
 * 2. Open this sketch in Arduino IDE
 * 3. Select Tools > Board > LOLIN(WEMOS) D1 mini (ESP8266)
 * 4. Select Tools > Upload Speed > 921600
 * 5. Upload and open Serial Monitor (115200 baud)
 */

#include <CharGraphTimeLogic.h>

// 110-character pattern (10 rows × 11 columns)
// Normal word order
const char PATTERN_NORMAL[] PROGMEM =
  "ESIST-FUENFZEHNVIERTELVOR"
  "NACHABFASTHALBALDZWEI----"
  "DREEINSIEBENEFL-ZWOELF"
  "FUENF-SECHSVIER-ZEHNEUNACHT"
  "-------UHR";

// Swapped pattern: NACH before VIERTEL (triggers fallback)
// This tests the multi-level fallback for :15-:19
const char PATTERN_SWAPPED[] PROGMEM =
  "ESIST-FUENFZEHN-NACH-VOR-"
  "VIERTELFASTHALBALDZWEI----"
  "DREEINSIEBENEFL-ZWOELF"
  "FUENF-SECHSVIER-ZEHNEUNACHT"
  "-------UHR";

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n=== CharGraph Time Logic - Basic Example ===\n");
}

void loop() {
  // Test 1: Normal pattern (reference)
  Serial.println("=== TEST 1: NORMAL PATTERN ===");
  testPattern(PATTERN_NORMAL, "NORMAL");
  testTime(PATTERN_NORMAL, 6, 0);    // 6:00 → ES IST SECHS
  testTime(PATTERN_NORMAL, 6, 5);    // 6:05 → ES IST FÜNF NACH SECHS
  testTime(PATTERN_NORMAL, 6, 15);   // 6:15 → ES IST VIERTEL NACH SECHS
  testTime(PATTERN_NORMAL, 6, 30);   // 6:30 → ES IST HALB SIEBEN
  testTime(PATTERN_NORMAL, 6, 45);   // 6:45 → ES IST VIERTEL VOR SIEBEN
  testTime(PATTERN_NORMAL, 6, 55);   // 6:55 → ES IST FÜNF VOR SIEBEN

  // Test 2: Swapped pattern (triggers fallback)
  Serial.println("\n=== TEST 2: SWAPPED PATTERN (Fallback Test) ===");
  testPattern(PATTERN_SWAPPED, "SWAPPED");
  testTime(PATTERN_SWAPPED, 1, 15);   // 1:15 → Fallback: ES IST VIERTEL EINS (no NACH)
  testTime(PATTERN_SWAPPED, 1, 16);   // 1:16 → Fallback: ES IST ZEHN VOR HALB ZWEI (4 LEDs LEFT)
  testTime(PATTERN_SWAPPED, 1, 17);   // 1:17 → Fallback: ES IST ZEHN VOR HALB ZWEI (3 LEDs LEFT)
  testTime(PATTERN_SWAPPED, 1, 18);   // 1:18 → Fallback: ES IST ZEHN VOR HALB ZWEI (2 LEDs LEFT)
  testTime(PATTERN_SWAPPED, 1, 19);   // 1:19 → Fallback: ES IST ZEHN VOR HALB ZWEI (1 LED LEFT)

  Serial.println("\n--- Test completed ---\n");
  delay(5000);  // Wait 5 seconds before next test
}

void testPattern(const char* pattern, const char* patternName) {
  Serial.print("Pattern: ");
  Serial.print(patternName);
  Serial.print(" (Length: ");
  Serial.print(strlen(pattern));
  Serial.println(")");
  delay(1000);
}

void testTime(const char* pattern, uint8_t hour, uint8_t minute) {
  Serial.print("  Time: ");
  if (hour < 10) Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.println(minute);

  CharGraphTimeWords result;

  // Call library function
  if (getCharGraphWords(pattern, hour, minute, result)) {
    // Success - print results
    Serial.print("    Text: ");
    Serial.println(result.text);

    Serial.print("    Words: ");
    for (uint8_t i = 0; i < result.wordCount; i++) {
      Serial.print((const __FlashStringHelper*) result.words[i]);
      if (i < result.wordCount - 1) Serial.print(" ");
    }
    Serial.println();

    Serial.print("    LED: Count=");
    Serial.print(result.ledCount);
    Serial.print(", Direction=");
    Serial.print((const __FlashStringHelper*) result.ledDirection);
    Serial.print(", Hex=0x");
    if (result.ledHex < 0x10) Serial.print("0");
    Serial.println(result.ledHex, HEX);
  } else {
    // Error - validation failed
    Serial.println("    ERROR: Validation failed!");
    delay(200);
  }
}
