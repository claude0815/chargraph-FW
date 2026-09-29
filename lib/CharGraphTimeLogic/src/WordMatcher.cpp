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
  LEDInfo& outLedInfo,
  int16_t* outPositions
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

  // ========== STEP 4: Calculate display hour (advance at mm >= 25) ==========
  // :20-:24 ZWANZIG NACH uses current hour
  // :25-:59 other rules use next hour
  uint8_t h12 = hour % 12;
  if (minute >= 25) {
    h12 = (h12 + 1) % 12;
  }

  const char* hourWord = getHourWord(h12);

  // ========== STEP 4b: SPECIAL CASE - NACHT (:00-:04 at hour 0 only) ==========
  // If NACHT is present and it's midnight (hour 0) and minute is 0-4, return only intro + NACHT (no minute words)
  // (intro words are already in outWords[0..1])
  if (hasNacht && hour == 0 && minute <= 4) {
    outWords[wordIdx++] = NACHT;

    // Validate NACHT sequence
    ValidationResult nachtValidation = validateWordSequence(outWords, wordIdx, pattern, outPositions);
    if (nachtValidation.valid) {
      // Minute LEDs for :01-:04 (LEFT = minutes passed)
      outLedInfo = calculateLEDs(outWords, wordIdx, minute);
      return wordIdx;
    }
    // NACHT not displayable in this order -> fall through to normal rules
    wordIdx = 2;
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

  // ========== STEP 6/7: Try primary rule, then fallbacks until valid ==========
  // Order: (modifiers, fb0) -> (no modifiers, fb0) -> (modifiers, fb1) ->
  //        (no modifiers, fb1) -> (modifiers, fb2) -> (no modifiers, fb2)
  // Each attempt starts from the original context, so a later fallback level
  // never re-enables modifiers that already failed. Only a validated word
  // sequence is returned; if nothing validates, the primary result is kept
  // (best effort, positions unknown) so the clock does not go dark.
  bool hasModifiers = hasKurz || hasBald || hasFast;
  const char* primaryWords[10];
  uint8_t primaryCount = 0;

  for (uint8_t attempt = 0; attempt < 6; attempt++) {
    bool withModifiers = (attempt % 2) == 0;
    if (!withModifiers && !hasModifiers) continue;  // identical to previous attempt

    RuleContext attemptCtx = ctx;
    attemptCtx.fallbackLevel = attempt / 2;
    if (!withModifiers) {
      attemptCtx.hasKurz = false;
      attemptCtx.hasBald = false;
      attemptCtx.hasFast = false;
    }

    const char* minuteWords[6];
    uint8_t minuteWordCount = executeMinuteRule(minute, attemptCtx, minuteWords);
    if (minuteWordCount == 0) continue;

    uint8_t totalWords = 2;  // intro words
    for (uint8_t i = 0; i < minuteWordCount && totalWords < 10; i++) {
      outWords[totalWords++] = minuteWords[i];
    }

    if (primaryCount == 0) {
      for (uint8_t i = 0; i < totalWords; i++) primaryWords[i] = outWords[i];
      primaryCount = totalWords;
    }

    ValidationResult validation = validateWordSequence(outWords, totalWords, pattern, outPositions);
    if (validation.valid) {
      // ========== STEP 8: Calculate LED info ==========
      outLedInfo = calculateLEDs(outWords, totalWords, minute);
      return totalWords;
    }
  }

  if (primaryCount == 0) {
    outLedInfo = {0, LEFT, 0x00};
    return 0;
  }

  for (uint8_t i = 0; i < primaryCount; i++) {
    outWords[i] = primaryWords[i];
    if (outPositions) outPositions[i] = -1;
  }
  outLedInfo = calculateLEDs(outWords, primaryCount, minute);
  return primaryCount;
}
