/**
 * CharGraph Time Logic - Word Matcher Implementation
 */

#include "WordMatcher.h"
#include "Constants.h"
#include "MinuteRules.h"
#include "LEDCalculator.h"
#include "Validator.h"
#include <cstring>

// ============================================================================
// HELPER: Find string in pattern (case-sensitive)
// ============================================================================

static int16_t findWord(const char* pattern, const char* word_progmem) {
  if (!pattern || !word_progmem) return -1;

  char wordBuf[12];  // Max 11 chars (DREIVIERTEL) + null terminator
  strcpy_P(wordBuf, word_progmem);

  const char* result = strstr(pattern, wordBuf);
  if (result) {
    return (result - pattern);
  }
  return -1;
}

// ============================================================================
// HELPER: Check if PROGMEM string is in pattern
// ============================================================================

static bool hasWord(const char* pattern, const char* word_progmem) {
  return findWord(pattern, word_progmem) != -1;
}

// ============================================================================
// MAIN FUNCTION: GET WORDS FOR TIME
// ============================================================================

uint8_t getWordsForTime(
  const char* pattern,
  uint8_t hour,
  uint8_t minute,
  const char** outWords,
  LEDInfo& outLedInfo
) {
  if (!pattern || !outWords) {
    outLedInfo = {0, LEFT, 0x00};
    return 0;
  }

  // ========== STEP 1: Recognize modifiers from pattern ==========
  bool hasKurz = hasWord(pattern, KURZ);
  bool hasBald = hasWord(pattern, BALD);
  bool hasFast = hasWord(pattern, FAST);
  bool hasZwanzig = hasWord(pattern, ZWANZIG);
  bool hasDreiviertel = hasWord(pattern, DREIVIERTEL);
  bool hasNacht = hasWord(pattern, NACHT);

  // ========== STEP 2: Detect alternative intro (WIR/HABEN vs ES/IST) ==========
  bool hasWir = hasWord(pattern, WIR);
  bool hasHaben = hasWord(pattern, HABEN);
  bool useAlternative = hasWir && hasHaben;

  uint8_t wordIdx = 0;

  if (useAlternative) {
    outWords[wordIdx++] = WIR;
    outWords[wordIdx++] = HABEN;
  } else {
    outWords[wordIdx++] = ES;
    outWords[wordIdx++] = IST;
  }

  // ========== STEP 3: Check for UHR at pattern end ==========
  int16_t uhrPos = findWord(pattern, UHR);
  bool hasUhrAtEnd = false;
  if (uhrPos != -1) {
    // Check if UHR is truly at the end (only placeholders after)
    bool afterUhrEmpty = true;
    for (uint16_t i = uhrPos + 3; pattern[i] != '\0'; i++) {
      if (pattern[i] >= 'A' && pattern[i] <= 'Z') {
        afterUhrEmpty = false;
        break;
      }
    }
    hasUhrAtEnd = afterUhrEmpty;
  }

  // ========== STEP 4: Calculate display hour (advance at mm >= 20) ==========
  uint8_t h12 = hour % 12;
  if (minute >= 20) {
    h12 = (h12 + 1) % 12;
  }

  const char* hourWord = getHourWord(h12);

  // ========== STEP 4b: SPECIAL CASE - NACHT (:00-:04 at hour 0 only) ==========
  // If NACHT is present and it's midnight (hour 0) and minute is 0-4, return only intro + NACHT (no minute words)
  if (hasNacht && hour == 0 && minute <= 4) {
    outWords[wordIdx++] = useAlternative ? WIR : ES;
    outWords[wordIdx++] = useAlternative ? HABEN : IST;
    outWords[wordIdx++] = NACHT;

    // Validate NACHT sequence
    ValidationResult nachtValidation = validateWordSequence(outWords, 3, pattern);
    if (!nachtValidation.valid) {
      outLedInfo = {0, LEFT, 0x00};
      return 0;
    }

    // Calculate LEDs (should be 0 for NACHT at :00-:04)
    outLedInfo = calculateLEDs(outWords, 3, minute);
    return 3;
  }

  // ========== STEP 5: Apply minute rule handler ==========
  RuleContext ctx;
  ctx.mm = minute;
  ctx.h12 = h12;
  ctx.hourWord = hourWord;
  ctx.hasUhrAtEnd = hasUhrAtEnd;
  ctx.hasKurz = hasKurz;
  ctx.hasBald = hasBald;
  ctx.hasFast = hasFast;
  ctx.hasZwanzig = hasZwanzig;
  ctx.hasDreiviertel = hasDreiviertel;
  ctx.hasNacht = hasNacht;
  ctx.gridStr = pattern;
  ctx.fallbackLevel = 0;  // Initialize with 0 (primary)

  const char* minuteWords[6];
  uint8_t minuteWordCount = executeMinuteRule(minute, ctx, minuteWords);

  if (minuteWordCount == 0) {
    outLedInfo = {0, LEFT, 0x00};
    return 0;
  }

  // Add minute words to output
  for (uint8_t i = 0; i < minuteWordCount; i++) {
    outWords[wordIdx++] = minuteWords[i];
  }

  uint8_t totalWords = wordIdx;

  // ========== STEP 6: Validate word sequence ==========
  ValidationResult validation = validateWordSequence(outWords, totalWords, pattern);

  // ========== STEP 7: Multi-Level Fallback if validation fails ==========
  if (!validation.valid) {
    bool fallbackSuccess = false;

    // Try up to 3 fallback levels
    for (uint8_t fbLevel = 1; fbLevel <= 3 && !fallbackSuccess; fbLevel++) {
      RuleContext ctxFallback = ctx;

      // Level 1: Remove modifiers (existing logic)
      if (fbLevel == 1 && (hasKurz || hasBald || hasFast)) {
        ctxFallback.hasKurz = false;
        ctxFallback.hasBald = false;
        ctxFallback.hasFast = false;
        ctxFallback.fallbackLevel = 0;  // Still use primary rule
      }
      // Level 2: Use rule's first fallback alternative
      else if (fbLevel == 2) {
        ctxFallback.fallbackLevel = 1;  // Signal first fallback
      }
      // Level 3: Use rule's second fallback (rare)
      else if (fbLevel == 3) {
        ctxFallback.fallbackLevel = 2;
      }
      else {
        continue;  // Skip this level
      }

      // Execute minute rule with fallback context
      const char* minuteWordsFallback[6];
      uint8_t minuteWordCountFallback = executeMinuteRule(minute, ctxFallback, minuteWordsFallback);

      if (minuteWordCountFallback > 0) {
        // Rebuild full word list
        wordIdx = 0;
        if (useAlternative) {
          outWords[wordIdx++] = WIR;
          outWords[wordIdx++] = HABEN;
        } else {
          outWords[wordIdx++] = ES;
          outWords[wordIdx++] = IST;
        }

        for (uint8_t i = 0; i < minuteWordCountFallback; i++) {
          outWords[wordIdx++] = minuteWordsFallback[i];
        }

        totalWords = wordIdx;

        // Re-validate
        ValidationResult validationFallback = validateWordSequence(outWords, totalWords, pattern);
        if (validationFallback.valid) {
          validation = validationFallback;
          fallbackSuccess = true;
        }
      }
    }
  }

  // ========== STEP 8: Calculate LED info ==========
  outLedInfo = calculateLEDs(outWords, totalWords, minute);

  return totalWords;
}
