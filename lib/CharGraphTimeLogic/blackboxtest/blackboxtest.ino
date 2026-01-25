/**
 * CharGraph TimeLogic - Black Box Test
 *
 * Automated test suite for CharGraphTimeLogic library
 * Tests multiple patterns across a configurable time window
 */

#include "config.h"
#include "../src/CharGraphTimeLogic.h"

// ============================================================================
// GLOBAL TEST STATISTICS
// ============================================================================

struct TestStats {
  uint32_t totalTests;
  uint32_t passedTests;
  uint32_t failedTests;
};

TestStats stats = {0, 0, 0};

// ============================================================================
// SETUP AND INITIALIZATION
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("CharGraph TimeLogic - Black Box Test");
  Serial.println("========================================\n");

  Serial.print("Test Patterns: ");
  Serial.println(numPatterns);
  Serial.print("Time Window: ");
  Serial.print(hourStart);
  Serial.print(":");
  if (minuteStart < 10) Serial.print("0");
  Serial.print(minuteStart);
  Serial.print(" to ");
  Serial.print(hourEnd);
  Serial.print(":");
  if (minuteEnd < 10) Serial.print("0");
  Serial.print(minuteEnd);
  Serial.println();
  Serial.print("Minute Hops: ");
  Serial.println(hops);
  Serial.println();

  // Run all tests
  for (uint8_t patternIdx = 0; patternIdx < numPatterns; patternIdx++) {
    runPatternTest(patternIdx, testpatterns[patternIdx]);
  }

  // Print summary
  printTestSummary();

  Serial.println("\nTest completed. Ready for next test.\n");
}

void loop() {
  // Loop does nothing - tests run in setup
  delay(1000);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

void runPatternTest(uint8_t patternIdx, const char* pattern) {
  Serial.println("========================================");
  Serial.print("Pattern ");
  Serial.print(patternIdx + 1);
  Serial.println();
  Serial.println("========================================");
  Serial.print("Pattern: ");
  Serial.println(pattern);

  // Verify pattern length
  if (strlen(pattern) != GRID_SIZE) {
    Serial.print("ERROR: Pattern length ");
    Serial.print(strlen(pattern));
    Serial.print(" != ");
    Serial.println(GRID_SIZE);
    return;
  }

  Serial.println("\nUhrzeit | Text | MinutenLeds | Links | Rechts");
  Serial.println("--------|------|-------------|-------|-------");

  // Calculate total test count
  uint16_t testCount = calculateTestCount();

  // Iterate through time window
  uint8_t hour = hourStart;
  uint8_t minute = minuteStart;
  uint16_t count = 0;

  while (count < testCount) {
    // Get words for this time
    CharGraphTimeWords result;
    bool success = getCharGraphWords(pattern, hour, minute, result);

    // Print result line
    printResultLine(hour, minute, result, success);

    // Update statistics
    stats.totalTests++;
    if (success) {
      stats.passedTests++;
    } else {
      stats.failedTests++;
    }

    // Advance time by hop increment
    minute += hops;
    if (minute > 59) {
      minute -= 60;
      hour++;
      if (hour > 23) {
        hour = 0;
      }
    }

    count++;
  }

  Serial.println();
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

uint16_t calculateTestCount() {
  /**
   * Calculate how many minutes will be tested based on:
   * - Start and end times
   * - Hop increment
   */

  uint16_t count = 0;
  uint8_t hour = hourStart;
  uint8_t minute = minuteStart;

  // Safety limit: maximum 10000 iterations to prevent infinite loops
  for (uint16_t i = 0; i < 10000; i++) {
    count++;

    // Check if we've reached the end
    if (hour == hourEnd && minute == minuteEnd) {
      break;
    }

    // Advance time
    minute += hops;
    if (minute > 59) {
      minute -= 60;
      hour++;
      if (hour > 23) {
        hour = 0;
      }
    }
  }

  return count;
}

void printResultLine(uint8_t hour, uint8_t minute, const CharGraphTimeWords& result, bool success) {
  /**
   * Print one result line in table format:
   * Uhrzeit | Text | MinutenLeds | Links | Rechts
   */

  // Uhrzeit
  if (hour < 10) Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.print(minute);
  Serial.print(" | ");

  // Text (first 37 chars max, padded)
  char textBuf[100];
  if (success) {
    buildTextFromWords(result.words, result.wordCount, textBuf);
  } else {
    strcpy(textBuf, "ERROR");
  }

  int len = strlen(textBuf);
  Serial.print(textBuf);
  for (int i = len; i < 37; i++) {
    Serial.print(" ");
  }
  Serial.print(" | ");

  // MinutenLeds (LED count display)
  if (result.ledCount > 0) {
    for (uint8_t i = 0; i < result.ledCount; i++) {
      Serial.print("*");  // Use * instead of emoji for reliability
    }
  }
  for (uint8_t i = result.ledCount; i < 11; i++) {
    Serial.print(" ");
  }
  Serial.print(" | ");

  // Links / Rechts
  if (result.ledCount > 0) {
    if (result.ledDirection == LEFT) {
      Serial.print("L");
      Serial.print("   | ");
      Serial.print("R");
    } else {
      Serial.print("R");
      Serial.print("   | ");
      Serial.print("L");
    }
  } else {
    Serial.print(" ");
    Serial.print("   | ");
    Serial.print(" ");
  }

  Serial.println();
}

void buildTextFromWords(const char* const* words, uint8_t wordCount, char* outText) {
  /**
   * Build readable text from word array
   */

  if (!words || !outText || wordCount == 0) {
    outText[0] = '\0';
    return;
  }

  uint16_t pos = 0;
  const uint16_t maxLen = 99;

  for (uint8_t i = 0; i < wordCount; i++) {
    if (i > 0 && pos < maxLen) {
      outText[pos++] = ' ';
    }

    // Load word from PROGMEM and copy
    char wordBuf[11];
    strcpy_P(wordBuf, words[i]);

    for (uint8_t j = 0; wordBuf[j] != '\0' && pos < maxLen; j++) {
      outText[pos++] = wordBuf[j];
    }
  }

  outText[pos] = '\0';
}

void printTestSummary() {
  /**
   * Print test statistics summary
   */

  Serial.println("\n========================================");
  Serial.println("TEST SUMMARY");
  Serial.println("========================================");
  Serial.print("Total Tests: ");
  Serial.println(stats.totalTests);
  Serial.print("Passed: ");
  Serial.println(stats.passedTests);
  Serial.print("Failed: ");
  Serial.println(stats.failedTests);

  if (stats.failedTests == 0) {
    Serial.println("\n✓ ALL TESTS PASSED");
  } else {
    Serial.print("\n✗ ");
    Serial.print(stats.failedTests);
    Serial.println(" TESTS FAILED");
  }
  Serial.println("========================================");
}
