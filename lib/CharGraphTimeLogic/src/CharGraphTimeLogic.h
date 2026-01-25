/**
 * CharGraph Time Logic Library for Arduino/ESP8266
 *
 * Converts German time (hour:minute) to word sequence and calculates
 * minute LED positions for word clock displays.
 *
 * Usage:
 *   #include <CharGraphTimeLogic.h>
 *
 *   const char pattern[] = "ESIST..."; // 110 chars
 *   CharGraphTimeWords result;
 *
 *   if (getCharGraphWords(pattern, 14, 30, result)) {
 *     Serial.println(result.text);  // "ES IST HALB DREI"
 *     Serial.println(result.ledHex); // 0x00
 *   }
 */

#ifndef CHARGRAPH_TIME_LOGIC_H
#define CHARGRAPH_TIME_LOGIC_H

#include <Arduino.h>

// ============================================================================
// PUBLIC TYPES
// ============================================================================

/**
 * Result structure for time-to-words conversion
 */
struct CharGraphTimeWords {
  const char* words[10];    // Array of word pointers (PROGMEM)
  uint8_t wordCount;        // Number of words (1-10)

  // LED Information
  uint8_t ledCount;         // 0-4 LEDs
  const char* ledDirection; // "left" or "right" (PROGMEM)
  uint8_t ledHex;           // 4-bit value (0x00-0x0F)

  // Text representation
  char text[100];           // Full text (optional, built by helper)
};

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Convert time to CharGraph words and LED positions
 *
 * @param pattern 110-character grid (uppercase, uppercase A-Z and 0-9)
 * @param hour Hour (0-23)
 * @param minute Minute (0-59)
 * @param outResult Result structure (filled on success)
 * @return true if successful, false on error (pattern validation failed)
 *
 * Example:
 *   const char pattern[] PROGMEM = "ESIST-FÜNFZEHN...";
 *   CharGraphTimeWords result;
 *   if (getCharGraphWords(pattern, 14, 25, result)) {
 *     Serial.print("Words: ");
 *     for (int i = 0; i < result.wordCount; i++) {
 *       Serial.print((const __FlashStringHelper*) pgm_read_ptr(&result.words[i]));
 *       Serial.print(" ");
 *     }
 *   }
 */
int8_t getCharGraphWords(
  const char* pattern,
  uint8_t hour,
  uint8_t minute,
  CharGraphTimeWords& outResult
);

/**
 * Build human-readable text from word array (optional helper)
 *
 * @param words Array of word pointers (PROGMEM strings)
 * @param wordCount Number of words
 * @param outText Output buffer (at least 100 bytes)
 * @return Length of generated text
 *
 * Example:
 *   char text[100];
 *   buildCharGraphText(result.words, result.wordCount, text);
 *   Serial.println(text);  // "ES IST HALB DREI"
 */
uint16_t buildCharGraphText(
  const char* const* words,
  uint8_t wordCount,
  char* outText
);

// ============================================================================
// DEBUG FUNCTIONS (Optional, can be disabled)
// ============================================================================

#ifdef CHARGRAPH_DEBUG

/**
 * Print validation error message to Serial
 * Only available if CHARGRAPH_DEBUG is defined
 */
void debugPrintValidationError(const char* gridStr);

/**
 * Print LED calculation details to Serial
 * Only available if CHARGRAPH_DEBUG is defined
 */
void debugPrintLEDInfo(const CharGraphTimeWords& result);

#endif

#endif // CHARGRAPH_TIME_LOGIC_H
