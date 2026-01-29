/**
 * CharGraph Time Logic - Validator Implementation
 */

#include "Validator.h"
#include "Constants.h"

// ============================================================================
// HELPER: Check if gap exists between two positions
// ============================================================================

static bool hasGap(uint16_t endIdxPrev, uint16_t startIdxNext) {
  const uint8_t rowPrev = endIdxPrev / GRID_COLS;
  const uint8_t rowNext = startIdxNext / GRID_COLS;
  // Gap exists if: different row OR gap in same row (>1 char distance)
  return rowNext > rowPrev || startIdxNext > endIdxPrev + 1;
}

// ============================================================================
// HELPER: Find string in PROGMEM pattern
// ============================================================================

static int16_t findWord(const char* pattern, const char* word_progmem, uint16_t searchStart) {
  if (!pattern || !word_progmem) return -1;

  // Load word from PROGMEM into buffer (max 11 chars: DREIVIERTEL)
  char wordBuf[12];  // 11 chars + null terminator
  strcpy_P(wordBuf, word_progmem);

  // Search from searchStart
  const char* result = strstr(pattern + searchStart, wordBuf);
  if (result) {
    return (result - pattern);
  }
  return -1;
}

// ============================================================================
// HELPER: Check if any word from wordsList appears after startPos in pattern
// ============================================================================

static bool containsWordAfter(const char* pattern, const char* wordsList, uint16_t startPos) {
  if (!pattern || !wordsList) return false;

  // Parse wordsList and check each word
  uint16_t wordStart = 0;
  for (uint16_t i = 0; wordsList[i] != '\0'; i++) {
    if (wordsList[i] == '-' || wordsList[i + 1] == '\0') {
      // Extract word
      uint16_t wordLen = (wordsList[i] == '-') ? (i - wordStart) : (i - wordStart + 1);
      char word[12];  // Max 11 chars + null terminator

      if (wordLen > 0 && wordLen < 12) {
        strncpy(word, &wordsList[wordStart], wordLen);
        word[wordLen] = '\0';

        // Check if this word appears after startPos
        if (findWord(pattern, (const char*)word, startPos) != -1) {
          return true;
        }
      }

      wordStart = i + 1;
    }
  }
  return false;
}

// ============================================================================
// WORD INTEGRITY: Check if word fits in line
// ============================================================================

bool wordFitsInLine(uint16_t startPos, uint8_t wordLen) {
  const uint8_t startRow = startPos / GRID_COLS;
  const uint8_t endRow = (startPos + wordLen - 1) / GRID_COLS;
  // Word fits if start and end are in same row
  return startRow == endRow;
}

// ============================================================================
// STRUCTURE VALIDATION (with Word Integrity Check)
// ============================================================================

ValidationResult validateStructure(const char* gridStr, const char* wordsList) {
  if (!gridStr || !wordsList) {
    return {false, ERR_NO_ES};
  }

  // ========== INTEGRITY CHECK: Words must not wrap over line boundaries ==========
  // Find entry words
  int16_t esPos = findWord(gridStr, ES, 0);
  int16_t wirPos = findWord(gridStr, WIR, 0);
  int16_t istPos = findWord(gridStr, IST, 0);
  int16_t habenPos = findWord(gridStr, HABEN, 0);
  int16_t halbPos = findWord(gridStr, HALB, 0);
  int16_t uhrPos = findWord(gridStr, UHR, 0);

  // Check ES integrity
  if (esPos != -1 && !wordFitsInLine(esPos, 2)) {  // ES = 2 chars
    return {false, ERR_NO_ES};  // Generic error - word integrity issue
  }

  // Check IST integrity
  if (istPos != -1 && !wordFitsInLine(istPos, 3)) {  // IST = 3 chars
    return {false, ERR_NO_IST};
  }

  // Check WIR integrity
  if (wirPos != -1 && !wordFitsInLine(wirPos, 3)) {  // WIR = 3 chars
    return {false, ERR_NO_WIR};
  }

  // Check HABEN integrity
  if (habenPos != -1 && !wordFitsInLine(habenPos, 5)) {  // HABEN = 5 chars
    return {false, ERR_NO_HABEN};
  }

  // Check HALB integrity
  if (halbPos != -1 && !wordFitsInLine(halbPos, 4)) {  // HALB = 4 chars
    return {false, ERR_NO_HALB};
  }

  // Check UHR integrity
  if (uhrPos != -1 && !wordFitsInLine(uhrPos, 3)) {  // UHR = 3 chars
    return {false, ERR_UHR_NOT_LAST};
  }

  // ========== INTRO WORDS VALIDATION ==========
  const bool useAlternative = (esPos == -1 && wirPos != -1);

  if (useAlternative) {
    // WIR HABEN variant
    if (wirPos == -1) {
      return {false, ERR_NO_WIR};
    }
    if (habenPos == -1) {
      return {false, ERR_NO_HABEN};
    }
    if (!hasGap(wirPos + 2, habenPos)) {
      return {false, ERR_NO_GAP_WIR_HABEN};
    }
  } else {
    // ES IST variant (standard)
    if (esPos == -1) {
      return {false, ERR_NO_ES};
    }
    if (istPos == -1) {
      return {false, ERR_NO_IST};
    }
    if (!hasGap(esPos + 1, istPos)) {
      return {false, ERR_NO_GAP_ES_IST};
    }
  }

  // ========== HALB VALIDATION ==========
  if (halbPos == -1) {
    return {false, ERR_NO_HALB};
  }

  // ========== UHR VALIDATION ==========
  if (uhrPos != -1) {
    // UHR is present - check if at end
    // After UHR, no words from wordsList should appear
    if (containsWordAfter(gridStr, wordsList, uhrPos + 3)) {
      return {false, ERR_UHR_NOT_LAST};
    }
  }

  return {true, nullptr};
}

// ============================================================================
// MANDATORY WORDS VALIDATION (with Word Integrity Check)
// ============================================================================

ValidationResult validateMandatoryWords(const char* gridStr) {
  if (!gridStr) {
    return {false, ERR_NO_FUENF};
  }

  // ========== INTEGRITY CHECK: Minute words must not wrap over line boundaries ==========

  // Check FÜNF (mandatory for minute display)
  int16_t fuenfPos = findWord(gridStr, FUENF, 0);
  if (fuenfPos == -1) {
    return {false, ERR_NO_FUENF};
  }
  if (!wordFitsInLine(fuenfPos, 4)) {  // FÜNF = 4 chars
    return {false, ERR_NO_FUENF};
  }

  // Check ZEHN (mandatory for minute display)
  int16_t zehnPos = findWord(gridStr, ZEHN, 0);
  if (zehnPos == -1) {
    return {false, ERR_NO_ZEHN};
  }
  if (!wordFitsInLine(zehnPos, 4)) {  // ZEHN = 4 chars
    return {false, ERR_NO_ZEHN};
  }

  // Check VIERTEL (mandatory)
  int16_t viertelPos = findWord(gridStr, VIERTEL, 0);
  if (viertelPos == -1) {
    return {false, ERR_NO_VIERTEL};
  }
  if (!wordFitsInLine(viertelPos, 7)) {  // VIERTEL = 7 chars
    return {false, ERR_NO_VIERTEL};
  }

  // Check VOR (mandatory)
  int16_t vorPos = findWord(gridStr, VOR, 0);
  if (vorPos == -1) {
    return {false, ERR_NO_VOR};
  }
  if (!wordFitsInLine(vorPos, 3)) {  // VOR = 3 chars
    return {false, ERR_NO_VOR};
  }

  // Check NACH (mandatory)
  int16_t nachPos = findWord(gridStr, NACH, 0);
  if (nachPos == -1) {
    return {false, ERR_NO_NACH};
  }
  if (!wordFitsInLine(nachPos, 4)) {  // NACH = 4 chars
    return {false, ERR_NO_NACH};
  }

  // ========== GAP VALIDATION ==========
  // Check if DREIVIERTEL is present (must check BEFORE VIERTEL gap validation)
  int16_t dreiviertelPos = findWord(gridStr, DREIVIERTEL, 0);
  bool hasDreiviertel = (dreiviertelPos != -1);

  // Check gap between VIERTEL and VOR
  // BUT: Skip this check if DREIVIERTEL is present (VIERTEL is substring of DREIVIERTEL)
  if (!hasDreiviertel && !hasGap(viertelPos + 6, vorPos)) {
    return {false, ERR_NO_GAP_VIERTEL_VOR};
  }

  // Check gap between VIERTEL and NACH
  // BUT: Skip this check if DREIVIERTEL is present (VIERTEL is substring of DREIVIERTEL)
  if (!hasDreiviertel && !hasGap(viertelPos + 6, nachPos)) {
    return {false, ERR_NO_GAP_VIERTEL_NACH};
  }

  // ========== CRITICAL SEQUENCE VALIDATION for :45 ==========
  // :45 requires EITHER DREIVIERTEL OR the sequence VIERTEL VOR (in that order)
  // If DREIVIERTEL is NOT present, VIERTEL MUST come BEFORE VOR
  // Otherwise :45 cannot be displayed (e.g., "quarter to X" can't be formed)
  if (!hasDreiviertel && viertelPos > vorPos) {
    // VIERTEL comes AFTER VOR → can't form VIERTEL VOR sequence
    // And DREIVIERTEL is missing → can't display :45 at all
    return {false, ERR_NO_VIERTEL_SEQUENCE};
  }

  return {true, nullptr};
}

// ============================================================================
// OPTIONAL WORDS VALIDATION (INFO ONLY, NO ERROR)
// ============================================================================

ValidationResult validateOptionalWord(
  const char* gridStr,
  const char* optionalWord,
  int16_t istPos
) {
  if (!gridStr || !optionalWord || istPos == -1) {
    return {true, nullptr};  // No error, just skip
  }

  // Find optional word in pattern
  int16_t optWordPos = findWord(gridStr, optionalWord, 0);
  if (optWordPos == -1) {
    // Optional word not present - that's OK, no warning
    return {true, nullptr};
  }

  // Optional word is present - check gap after IST/HABEN
  // IST is 3 chars, HABEN is 5 chars
  int16_t gapStart = istPos + 3;  // Assuming IST (3 chars)

  // Check if there's a gap (should be different row or at least 1 char distance)
  if (!hasGap(gapStart - 1, optWordPos)) {
    // No gap - return warning based on which word
    if (wordEquals(optionalWord, "NACHT")) {
      return {true, WARN_NO_GAP_NACHT};
    } else if (wordEquals(optionalWord, "ZEIT")) {
      return {true, WARN_NO_GAP_ZEIT};
    } else if (wordEquals(optionalWord, "ALARM")) {
      return {true, WARN_NO_GAP_ALARM};
    } else if (wordEquals(optionalWord, "PAUSE")) {
      return {true, WARN_NO_GAP_PAUSE};
    } else if (wordEquals(optionalWord, "RWD")) {
      return {true, WARN_NO_GAP_RWD};
    }
  }

  return {true, nullptr};
}

// ============================================================================
// WORD SEQUENCE VALIDATION
// ============================================================================

ValidationResult validateWordSequence(
  const char* const* words,
  uint8_t wordCount,
  const char* gridStr
) {
  if (!words || wordCount == 0) {
    return {false, ERR_NO_WORDS};
  }

  // Find all words sequentially
  uint16_t searchStart = 0;
  uint16_t positions[10];  // Max 10 words
  uint16_t positionEnds[10];

  for (uint8_t i = 0; i < wordCount; i++) {
    int16_t pos = findWord(gridStr, words[i], searchStart);

    if (pos == -1) {
      return {false, ERR_WORD_NOT_FOUND};
    }

    // Load word from PROGMEM to get length
    char wordBuf[12];  // Max 11 chars (DREIVIERTEL) + null terminator
    strcpy_P(wordBuf, words[i]);
    uint8_t wordLen = strlen(wordBuf);

    positions[i] = pos;
    positionEnds[i] = pos + wordLen - 1;

    // Next search starts after this word
    searchStart = pos + wordLen;
  }

  // Check gaps between words
  for (uint8_t i = 0; i < wordCount - 1; i++) {
    const uint16_t currentEnd = positionEnds[i];
    const uint16_t nextStart = positions[i + 1];

    const uint8_t currentRow = currentEnd / GRID_COLS;
    const uint8_t nextRow = nextStart / GRID_COLS;

    // If same row, must have gap (placeholder)
    if (currentRow == nextRow && nextStart == currentEnd + 1) {
      return {false, ERR_NO_GAP};
    }
  }

  return {true, nullptr};
}
