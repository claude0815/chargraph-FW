/**
 * CharGraph Time Logic - Minute Range Test
 *
 * Tests a full minute range with detailed LED and word output
 * Time Range: 12:29 to 13:35 (67 minutes)
 *
 * Output Format:
 * Uhrzeit | Text                     | MinutenLeds | Links | Rechts
 * 12:29   | ES IST VOR HALB EINS     | ✅✅✅✅    |  ❌   |   ✅
 * 12:30   | ES IST HALB DREI         |            |  ❌   |   ❌
 *
 * Tested on: WEMOS D1 Mini (ESP8266)
 *
 * Instructions:
 * 1. Install CharGraphTimeLogic library in Arduino/libraries/
 * 2. Open this sketch in Arduino IDE
 * 3. Select Tools > Board > LOLIN(WEMOS) D1 mini (ESP8266)
 * 4. Upload and open Serial Monitor (115200 baud)
 * 5. Copy the table output
 */

#include <CharGraphTimeLogic.h>

// 110-character pattern for testing
// User's pattern: "ESGISTWZEHNFÜNFVIERTELVORNACHBALDHALBKZWEINSYWAYOLOVDCQZEHNEUNACHTELFÜNFZWÖLFSECHSIEBENGEDREIVIERTMDCPAUSEUHRW"
const char TEST_PATTERN[] PROGMEM =
  "ESGISTWZEHNFÜNFVIERTEL"
  "VORNACHBALDHALBKZWEINS"
  "YWAYOLOVDCQZEHNEUNACHT"
  "ELFÜNFZWÖLFSECHSIEBENGED"
  "REIVIERTMDCPAUSEUHRW";

// Test result structure
struct TestResult {
  uint8_t hour;
  uint8_t minute;
  char text[100];
  uint8_t ledCount;
  const char* ledDirection;
  uint8_t ledHex;
  boolean success;
};

// Storage for all test results (67 minutes)
TestResult results[68];
uint8_t resultCount = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n=== CharGraph Time Logic - Minute Range Test ===");
  Serial.println("Time Range: 12:29 to 13:35");
  Serial.println("Pattern Length: 110 characters\n");

  // Validate pattern length
  if (strlen_P(TEST_PATTERN) != 110) {
    Serial.print("ERROR: Pattern length is ");
    Serial.print(strlen_P(TEST_PATTERN));
    Serial.println(" (expected 110)");
  }

  // Run all tests
  runMinuteRangeTest();

  // Print results table
  printResultsTable();

  Serial.println("\n=== Test Completed ===\n");
}

void loop() {
  // Nothing to do after setup
  delay(1000);
}

void runMinuteRangeTest() {
  Serial.println("Running tests...");

  uint8_t startHour = 12;
  uint8_t startMinute = 29;
  uint8_t endHour = 13;
  uint8_t endMinute = 35;

  uint8_t hour = startHour;
  uint8_t minute = startMinute;

  while (resultCount < 68) {
    // Test this time
    TestResult result;
    result.hour = hour;
    result.minute = minute;

    CharGraphTimeWords timeWords;

    // Call library function
    if (getCharGraphWords(TEST_PATTERN, hour, minute, timeWords)) {
      result.success = true;
      strcpy(result.text, timeWords.text);
      result.ledCount = timeWords.ledCount;
      result.ledDirection = timeWords.ledDirection;
      result.ledHex = timeWords.ledHex;
    } else {
      result.success = false;
      strcpy(result.text, "ERROR");
      result.ledCount = 0;
      result.ledDirection = "left";
      result.ledHex = 0;
    }

    results[resultCount++] = result;

    // Move to next minute
    minute++;
    if (minute > 59) {
      minute = 0;
      hour++;
    }

    // Stop after 13:35
    if (hour == endHour && minute > endMinute) {
      break;
    }
  }

  Serial.print("Tested ");
  Serial.print(resultCount);
  Serial.println(" minutes");
}

void printResultsTable() {
  Serial.println("\n=== RESULTS TABLE ===\n");

  // Header
  Serial.println("Uhrzeit | Text                     | MinutenLeds | Links | Rechts");
  Serial.println("--------|--------------------------|-------------|-------|-------");

  // Data rows
  for (uint8_t i = 0; i < resultCount; i++) {
    TestResult& res = results[i];

    // Uhrzeit
    if (res.hour < 10) Serial.print("0");
    Serial.print(res.hour);
    Serial.print(":");
    if (res.minute < 10) Serial.print("0");
    Serial.print(res.minute);
    Serial.print(" | ");

    // Text (truncate to 24 characters)
    char textBuf[25];
    strncpy(textBuf, res.text, 24);
    textBuf[24] = '\0';

    // Pad to 24 characters
    for (int j = strlen(textBuf); j < 24; j++) {
      textBuf[j] = ' ';
    }
    textBuf[24] = '\0';

    Serial.print(textBuf);
    Serial.print(" | ");

    // MinutenLeds (LED visualization)
    if (res.ledCount > 0) {
      for (uint8_t led = 0; led < res.ledCount; led++) {
        Serial.print("✅");
      }
      // Pad with spaces if less than 4 LEDs
      for (uint8_t space = res.ledCount; space < 4; space++) {
        Serial.print("  ");
      }
    } else {
      Serial.print("         ");  // 9 spaces for no LEDs
    }
    Serial.print(" | ");

    // Links (LEFT direction)
    if (res.success && strcmp_P(res.ledDirection, PSTR("left")) == 0 && res.ledCount > 0) {
      Serial.print("✅  ");
    } else {
      Serial.print("❌  ");
    }
    Serial.print(" | ");

    // Rechts (RIGHT direction)
    if (res.success && strcmp_P(res.ledDirection, PSTR("right")) == 0 && res.ledCount > 0) {
      Serial.print("✅");
    } else {
      Serial.print("❌");
    }

    Serial.println();
  }
}
