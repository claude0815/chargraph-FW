/**
 * CharGraph Time Logic Library - Main Implementation
 */

#include "CharGraphTimeLogic.h"
#include "WordMatcher.h"
#include "Validator.h"
#include "Constants.h"
#include <cstring>

// ============================================================================
// PUBLIC API: GET CHARGRAPH WORDS
// ============================================================================

int8_t getCharGraphWords(
  const char* pattern,
  uint8_t hour,
  uint8_t minute,
  CharGraphTimeWords& outResult
) {
  if (!pattern) {
    outResult.wordCount = 0;
    return -1;
  }

  // Validate pattern length
  if (strlen(pattern) != GRID_SIZE) {
    outResult.wordCount = 0;
    return -2;
  }

  // Validate input ranges
  if (hour > 23 || minute > 59) {
    outResult.wordCount = 0;
    return -3;
  }

  // Validate pattern structure (ES/IST, HALB, UHR placement)
  ValidationResult structValidation = validateStructure(pattern);
  if (!structValidation.valid) {
    outResult.wordCount = 0;
    if (structValidation.reason) {
#ifdef CHARGRAPH_DEBUG
      Serial.print("Structure validation failed: ");
      Serial.println((const __FlashStringHelper*) structValidation.reason);
#endif
    }
    return -4;
  }

  // Validate mandatory words (FÜNF, ZEHN, VIERTEL, VOR, NACH with gaps)
  ValidationResult mandatoryValidation = validateMandatoryWords(pattern);
  if (!mandatoryValidation.valid) {
    outResult.wordCount = 0;
    if (mandatoryValidation.reason) {
#ifdef CHARGRAPH_DEBUG
      Serial.print("Mandatory words validation failed: ");
      Serial.println((const __FlashStringHelper*) mandatoryValidation.reason);
#endif
    }
    return -5;
  }

  // Get words for time
  LEDInfo ledInfo;
  const char* words[10];

  uint8_t wordCount = getWordsForTime(
    pattern,
    hour,
    minute,
    words,
    ledInfo
  );

  if (wordCount == 0) {
    outResult.wordCount = 0;
    return -6;
  }

  // Fill result structure
  for (uint8_t i = 0; i < wordCount; i++) {
    outResult.words[i] = words[i];
  }
  outResult.wordCount = wordCount;
  outResult.ledCount = ledInfo.count;
  outResult.ledDirection = ledInfo.direction;
  outResult.ledHex = ledInfo.hex;

  // Build text representation
  buildCharGraphText(outResult.words, outResult.wordCount, outResult.text);

  return 0;
}

// ============================================================================
// HELPER: BUILD TEXT FROM WORDS
// ============================================================================

uint16_t buildCharGraphText(
  const char* const* words,
  uint8_t wordCount,
  char* outText
) {
  if (!words || !outText || wordCount == 0) {
    outText[0] = '\0';
    return 0;
  }

  uint16_t pos = 0;
  const uint16_t maxLen = 99;  // Leave room for null terminator

  for (uint8_t i = 0; i < wordCount; i++) {
    if (i > 0 && pos < maxLen) {
      outText[pos++] = ' ';
    }

    // Load word from PROGMEM
    char wordBuf[12];  // Max 11 chars (DREIVIERTEL) + null terminator
    strcpy_P(wordBuf, words[i]);

    // Copy word to output
    for (uint8_t j = 0; wordBuf[j] != '\0' && pos < maxLen; j++) {
      outText[pos++] = wordBuf[j];
    }
  }

  outText[pos] = '\0';
  return pos;
}

// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

#ifdef CHARGRAPH_DEBUG

void debugPrintValidationError(const char* gridStr) {
  if (!gridStr) return;

  ValidationResult result = validateStructure(gridStr);

  if (!result.valid) {
    Serial.print("Validation Error: ");
    if (result.reason) {
      Serial.println((const __FlashStringHelper*) result.reason);
    } else {
      Serial.println("Unknown error");
    }
  } else {
    Serial.println("Pattern structure is valid");
  }
}

void debugPrintLEDInfo(const CharGraphTimeWords& result) {
  Serial.print("LED Count: ");
  Serial.println(result.ledCount);

  Serial.print("LED Direction: ");
  Serial.println((const __FlashStringHelper*) result.ledDirection);

  Serial.print("LED Hex: 0x");
  if (result.ledHex < 0x10) Serial.print("0");
  Serial.println(result.ledHex, HEX);
}

#endif  // CHARGRAPH_DEBUG
