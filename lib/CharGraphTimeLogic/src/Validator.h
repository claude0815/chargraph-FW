/**
 * CharGraph Time Logic - Validator
 *
 * Port of lib/time-logic/validator.ts
 * Pattern structure validation and word sequence validation
 */

#ifndef CHARGRAPH_VALIDATOR_H
#define CHARGRAPH_VALIDATOR_H

#include "Types.h"

/**
 * Check if word fits completely in one 11-character line (no wrap)
 *
 * @param startPos Starting position of word
 * @param wordLen Length of word
 * @return true if word fits in line, false if wraps over boundary
 */
bool wordFitsInLine(uint16_t startPos, uint8_t wordLen);

/**
 * Validate pattern structure (entry words + word integrity)
 *
 * Checks:
 * - All words fit completely in one 11-character line (no wrap!)
 * - ES/IST or WIR/HABEN with gap between
 * - HALB is required
 * - UHR is optional, but if present must be at pattern end
 *
 * @param gridStr 110-character pattern (uppercase)
 * @return ValidationResult with valid flag and error message
 */
ValidationResult validateStructure(const char* gridStr);

/**
 * Validate mandatory words presence and gaps
 *
 * Checks all mandatory words that MUST be in every pattern:
 * - FÜNF (mandatory for minute display)
 * - ZEHN (mandatory for minute display)
 * - VIERTEL (mandatory, must have gap before VOR/NACH)
 * - VOR (mandatory)
 * - NACH (mandatory)
 * - Gaps between VIERTEL and VOR, VIERTEL and NACH
 *
 * @param gridStr 110-character pattern (uppercase)
 * @return ValidationResult with valid flag and error message
 */
ValidationResult validateMandatoryWords(const char* gridStr);

/**
 * Validate optional words presence and gaps
 *
 * Info function (no error, just warning) to check optional words:
 * - NACHT, ZEIT, ALARM, PAUSE, RWD
 * - Must have gap between IST/HABEN and optional word
 *
 * @param gridStr 110-character pattern (uppercase)
 * @param optionalWord PROGMEM pointer to optional word to check (e.g. NACHT)
 * @param istPos Position of IST or HABEN in pattern
 * @return ValidationResult with valid flag and optional warning message
 */
ValidationResult validateOptionalWord(
  const char* gridStr,
  const char* optionalWord,
  int16_t istPos
);

/**
 * Validate word sequence in pattern
 *
 * Checks:
 * - All words found sequentially in pattern
 * - Gaps/placeholders between words in same row
 *
 * @param words Array of word pointers (PROGMEM strings)
 * @param wordCount Number of words
 * @param gridStr 110-character pattern
 * @return ValidationResult with valid flag
 */
ValidationResult validateWordSequence(
  const char* const* words,
  uint8_t wordCount,
  const char* gridStr
);

#endif // CHARGRAPH_VALIDATOR_H
