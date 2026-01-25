/**
 * CharGraph Time Logic - Data Types
 *
 * TypeScript→C++ Port of lib/time-logic/types.ts
 * Optimized for ESP8266 with minimal memory footprint
 */

#ifndef CHARGRAPH_TYPES_H
#define CHARGRAPH_TYPES_H

#include <Arduino.h>

// ============================================================================
// VALIDATION RESULT
// ============================================================================

struct ValidationResult {
  bool valid;
  const char* reason;  // Points to PROGMEM string
};

// ============================================================================
// MINUTE RULE CONTEXT
// ============================================================================

struct RuleContext {
  uint8_t mm;              // Minute (0-59)
  uint8_t h12;             // Hour 12-format (0-11)
  const char* hourWord;    // Points to PROGMEM hour word
  bool hasUhrAtEnd;        // UHR present at pattern end

  // Optional modifiers
  bool hasKurz;            // Valid at :01-02, :28, :31-32, :58, :59
  bool hasBald;            // Valid at :27-29, :57-59
  bool hasFast;            // Valid at :29, :59
  bool hasZwanzig;         // Alternative for :20-24, :40-44
  bool hasDreiviertel;     // Alternative for :45
  bool hasNacht;           // Night indicator (optional)

  const char* gridStr;     // Pattern (110 chars)

  // Fallback control (NEW)
  uint8_t fallbackLevel;   // 0 = primary, 1+ = fallback levels
};

// ============================================================================
// LED INFO
// ============================================================================

struct LEDInfo {
  uint8_t count;           // 0-4 LEDs
  const char* direction;   // "left" or "right" (PROGMEM)
  uint8_t hex;             // 4-bit value (0x00-0x0F)
};

// ============================================================================
// TIME WORDS RESPONSE
// ============================================================================

struct TimeWordsResponse {
  const char** words;      // Array of word pointers (PROGMEM)
  uint8_t wordCount;       // Number of words (max 10)
  LEDInfo ledInfo;
  uint8_t ledHex;          // Same as ledInfo.hex
};

#endif // CHARGRAPH_TYPES_H
