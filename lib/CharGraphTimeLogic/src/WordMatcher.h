/**
 * CharGraph Time Logic - Word Matcher
 *
 * Port of lib/time-logic/word-matcher.ts
 * Main orchestration: getWordsForTime() function
 */

#ifndef CHARGRAPH_WORD_MATCHER_H
#define CHARGRAPH_WORD_MATCHER_H

#include "Types.h"

/**
 * Convert time (hour:minute) to word sequence
 *
 * Main algorithm:
 * 1. Recognize modifiers from pattern (KURZ, BALD, FAST, ZWANZIG, DREIVIERTEL, NACHT)
 * 2. Detect alternative intro (WIR/HABEN vs ES/IST)
 * 3. Check for UHR at pattern end
 * 4. Calculate display hour (advance at mm >= 20)
 * 5. Apply minute rule handler
 * 6. Validate word sequence in pattern
 * 7. Fallback: if validation fails, remove optional modifiers and retry
 *
 * @param pattern 110-character grid (uppercase)
 * @param hour Hour (0-23)
 * @param minute Minute (0-59)
 * @param outWords Output array for words (up to 10 words)
 * @param outLedInfo Output LED information
 * @return Number of words, or 0 on error
 */
uint8_t getWordsForTime(
  const char* pattern,
  uint8_t hour,
  uint8_t minute,
  const char** outWords,
  LEDInfo& outLedInfo
);

#endif // CHARGRAPH_WORD_MATCHER_H
