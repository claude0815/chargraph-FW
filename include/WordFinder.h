/**
 * CharGraph - Unified Word Finder
 *
 * Central component for finding words in patterns
 * Handles both PROGMEM patterns (for validation) and regular strings (for display)
 * Shared between CharGraphTimeLogic and rgbPanel
 */

#ifndef WORD_FINDER_H
#define WORD_FINDER_H

#include <Arduino.h>

// ============================================================================
// PROGMEM WORD SEARCH (for validation)
// ============================================================================

/**
 * Find word in PROGMEM pattern starting from position
 * Both pattern and word are in PROGMEM
 *
 * @param pattern_progmem Pattern string in PROGMEM
 * @param word_progmem Word pointer in PROGMEM
 * @param searchStart Start position for search (default 0)
 * @return Position of word, or -1 if not found
 */
inline int16_t findWordProgmem(const char* pattern_progmem, const char* word_progmem, uint16_t searchStart = 0) {
  if (!pattern_progmem || !word_progmem) return -1;

  // Load word from PROGMEM into buffer (max 11 chars: DREIVIERTEL)
  char wordBuf[12];  // 11 chars + null terminator
  strcpy_P(wordBuf, word_progmem);
  uint8_t wordLen = strlen(wordBuf);

  // Search through pattern_progmem byte by byte
  uint16_t pos = searchStart;
  while (true) {
    char patByte = pgm_read_byte(pattern_progmem + pos);
    if (patByte == '\0') break;  // End of pattern

    // Check if word matches at this position
    bool match = true;
    for (uint8_t i = 0; i < wordLen; i++) {
      char p = pgm_read_byte(pattern_progmem + pos + i);
      if (p != wordBuf[i]) {
        match = false;
        break;
      }
    }

    if (match) return pos;
    pos++;
  }

  return -1;
}

// ============================================================================
// STRING WORD SEARCH (for display)
// ============================================================================

/**
 * Find word in regular string with occurrence support
 *
 * @param text String to search in
 * @param word Word to find
 * @param occurrence Which occurrence to find (0 = first, 1 = second, etc.)
 * @param searchBackward Search backward from end
 * @return Position of word, or -1 if not found
 */
inline int findWordString(const char* text, const char* word, int occurrence = 0, bool searchBackward = false) {
  if (!text || !word) return -1;

  const int wordLen = strlen(word);
  const int textLen = strlen(text);

  if (searchBackward) {
    // Backward search: from end to beginning
    int foundCount = 0;
    for (int pos = textLen - wordLen; pos >= 0; --pos) {
      bool match = true;
      for (int j = 0; j < wordLen; ++j) {
        unsigned char c1 = text[pos + j];
        unsigned char c2 = word[j];

        // Lowercase comparison (case-insensitive)
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');

        if (c1 != c2) {
          match = false;
          break;
        }
      }

      if (match) {
        if (foundCount == occurrence) {
          return pos;
        }
        foundCount++;
      }
    }
  } else {
    // Forward search: from beginning to end
    int foundCount = 0;
    for (int pos = 0; pos <= textLen - wordLen; ++pos) {
      bool match = true;
      for (int j = 0; j < wordLen; ++j) {
        unsigned char c1 = text[pos + j];
        unsigned char c2 = word[j];

        // Lowercase comparison (case-insensitive)
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');

        if (c1 != c2) {
          match = false;
          break;
        }
      }

      if (match) {
        if (foundCount == occurrence) {
          return pos;
        }
        foundCount++;
      }
    }
  }

  return -1;
}

#endif // WORD_FINDER_H
