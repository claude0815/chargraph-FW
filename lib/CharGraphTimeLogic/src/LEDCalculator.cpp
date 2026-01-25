/**
 * CharGraph Time Logic - LED Calculator Implementation
 */

#include "LEDCalculator.h"
#include "Constants.h"
#include <cstring>

// ============================================================================
// TARGET MINUTE CALCULATION
// ============================================================================

uint8_t getTargetMinute(
  const char* const* words,
  uint8_t wordCount,
  bool& isLeftDirection
) {
  // Default: left (NACH = count up)
  isLeftDirection = true;

  // Look for minute-indicator words (FÜNF, ZEHN, VIERTEL, ZWANZIG, DREIVIERTEL)
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "FuNF")) {
      // FÜNF can be NACH (5 min) or VOR (55 min)
      for (uint8_t j = i + 1; j < wordCount; j++) {
        if (wordEquals(words[j], "VOR")) {
          isLeftDirection = false;
          return 55;  // FÜNF VOR = :55
        }
        if (wordEquals(words[j], "NACH")) {
          isLeftDirection = true;
          return 5;   // FÜNF NACH = :05
        }
      }
      // Default for FÜNF alone (rare)
      return 5;
    }
    if (wordEquals(words[i], "ZEHN")) {
      // ZEHN can be NACH (10 min) or VOR (50 min)
      for (uint8_t j = i + 1; j < wordCount; j++) {
        if (wordEquals(words[j], "VOR")) {
          isLeftDirection = false;
          return 50;  // ZEHN VOR = :50
        }
        if (wordEquals(words[j], "NACH")) {
          isLeftDirection = true;
          return 10;  // ZEHN NACH = :10
        }
      }
      return 10;
    }
    if (wordEquals(words[i], "VIERTEL")) {
      // VIERTEL is NACH (15 min) or VOR (45 min)
      for (uint8_t j = i + 1; j < wordCount; j++) {
        if (wordEquals(words[j], "VOR")) {
          isLeftDirection = false;
          return 45;  // VIERTEL VOR = :45
        }
        if (wordEquals(words[j], "NACH")) {
          isLeftDirection = true;
          return 15;  // VIERTEL NACH = :15
        }
      }
      return 15;
    }
    if (wordEquals(words[i], "ZWANZIG")) {
      // ZWANZIG is NACH (20 min) or VOR (40 min)
      for (uint8_t j = i + 1; j < wordCount; j++) {
        if (wordEquals(words[j], "VOR")) {
          isLeftDirection = false;
          return 40;  // ZWANZIG VOR = :40
        }
        if (wordEquals(words[j], "NACH")) {
          isLeftDirection = true;
          return 20;  // ZWANZIG NACH = :20
        }
      }
      return 20;
    }
    if (wordEquals(words[i], "DREIVIERTEL")) {
      // DREIVIERTEL is always :45
      isLeftDirection = true;
      return 45;
    }
  }

  // Check for HALB
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "HALB")) {
      // HALB NACH or HALB VOR?
      for (uint8_t j = i + 1; j < wordCount; j++) {
        if (wordEquals(words[j], "VOR")) {
          isLeftDirection = false;
          return 20;  // Something like "... VOR HALB" → target :20
        }
        if (wordEquals(words[j], "NACH")) {
          isLeftDirection = true;
          return 35;  // Something like "... NACH HALB" → target :35
        }
      }
      // HALB at :30
      return 30;
    }
  }

  return 0;
}

// ============================================================================
// LED COUNT TO HEX
// ============================================================================

uint8_t ledCountToHex(uint8_t count, bool isLeft) {
  count = constrain(count, 0, 4);

  if (isLeft) {
    // LEFT: additive from top (bits 3→0)
    // 0 LEDs: 0000 = 0x00
    // 1 LED:  1000 = 0x08
    // 2 LEDs: 1100 = 0x0C
    // 3 LEDs: 1110 = 0x0E
    // 4 LEDs: 1111 = 0x0F
    const uint8_t leftTable[5] = {0x00, 0x08, 0x0C, 0x0E, 0x0F};
    return leftTable[count];
  } else {
    // RIGHT: additive from bottom (bits 0→3)
    // 0 LEDs: 0000 = 0x00
    // 1 LED:  0001 = 0x01
    // 2 LEDs: 0011 = 0x03
    // 3 LEDs: 0111 = 0x07
    // 4 LEDs: 1111 = 0x0F
    const uint8_t rightTable[5] = {0x00, 0x01, 0x03, 0x07, 0x0F};
    return rightTable[count];
  }
}

// ============================================================================
// LED CALCULATION (11-TIER PRIORITY)
// ============================================================================

LEDInfo calculateLEDs(
  const char* const* words,
  uint8_t wordCount,
  uint8_t mm
) {
  LEDInfo result = {0, LEFT, 0x00};

  if (!words || wordCount == 0) {
    return result;
  }

  uint8_t remainder = 0;
  bool isLeftDir = true;

  // ===== PRIORITY 0: NACHT (special - Midnight context) =====
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "NACHT")) {
      // NACHT at :00-:04 - additive counting after midnight
      remainder = mm;  // 0, 1, 2, 3, 4
      if (remainder > 4) remainder = 0;
      isLeftDir = true;
      result.direction = LEFT;
      result.count = remainder;
      result.hex = ledCountToHex(remainder, isLeftDir);
      return result;
    }
  }

  // ===== PRIORITY 1: DREIVIERTEL =====
  // Only at :45, shows remainder towards :50
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "DREIVIERTEL")) {
      remainder = mm - 45;
      if (remainder > 4) remainder = 0;  // Out of valid range
      isLeftDir = true;
      result.direction = LEFT;
      result.count = remainder;
      result.hex = ledCountToHex(remainder, isLeftDir);
      return result;
    }
  }

  // ===== PRIORITY 2-3: BALD/FAST =====
  bool hasBald = false;
  bool hasFast = false;
  bool hasHalb = false;

  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "BALD")) hasBald = true;
    if (wordEquals(words[i], "FAST")) hasFast = true;
    if (wordEquals(words[i], "HALB")) hasHalb = true;
  }

  if (hasFast || hasBald) {
    if (hasHalb) {
      // PRIORITY 2: BALD/FAST + HALB → remainder = target - mm, direction = right
      bool dummy;
      uint8_t target = getTargetMinute(words, wordCount, dummy);
      remainder = (target > mm) ? (target - mm) : 0;
      if (remainder > 4) remainder = 0;
      isLeftDir = false;
    } else {
      // PRIORITY 3: BALD/FAST (no HALB, :57-59) → remainder = 60 - mm, direction = right
      remainder = 60 - mm;
      if (remainder > 4) remainder = 0;
      isLeftDir = false;
    }
    result.direction = RIGHT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 4-5: KURZ VOR =====
  bool hasKurz = false;
  bool hasVor = false;
  bool hasZehn = false;

  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "KURZ")) hasKurz = true;
    if (wordEquals(words[i], "VOR")) hasVor = true;
    if (wordEquals(words[i], "ZEHN")) hasZehn = true;
  }

  if (hasKurz && hasVor) {
    if (hasHalb) {
      // PRIORITY 4: KURZ VOR HALB → remainder = target - mm, direction = right
      bool dummy;
      uint8_t target = getTargetMinute(words, wordCount, dummy);
      remainder = (target > mm) ? (target - mm) : 0;
      if (remainder > 4) remainder = 0;
    } else {
      // PRIORITY 5: KURZ VOR (no HALB, :58) → remainder = 60 - mm, direction = right
      remainder = 60 - mm;
      if (remainder > 4) remainder = 0;
    }
    isLeftDir = false;
    result.direction = RIGHT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 6-7: KURZ NACH =====
  bool hasNach = false;
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "NACH")) hasNach = true;
  }

  if (hasKurz && hasNach) {
    if (hasHalb) {
      // PRIORITY 7: KURZ NACH HALB → remainder = mm % 5, direction = left
      remainder = mm % 5;
    } else {
      // PRIORITY 6: KURZ NACH (no HALB, :01-02) → remainder = mm % 5, direction = left
      remainder = mm % 5;
    }
    isLeftDir = true;
    result.direction = LEFT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 8: NACH (no HALB) =====
  if (hasNach && !hasHalb) {
    bool dummy;
    uint8_t target = getTargetMinute(words, wordCount, dummy);
    remainder = (mm > target) ? (mm - target) : 0;
    if (remainder > 4) remainder = 0;
    isLeftDir = true;
    result.direction = LEFT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 9-10: HALB + NACH/VOR =====
  if (hasHalb && hasNach) {
    // PRIORITY 9: HALB + NACH → remainder = mm % 5, direction = left
    remainder = mm % 5;
    isLeftDir = true;
    result.direction = LEFT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 10b: SPECIAL CASE :16-:19 FALLBACK ZEHN VOR HALB =====
  if (hasHalb && hasVor && hasZehn && mm >= 16 && mm <= 19) {
    // Fallback scenario: :16-:19 with "ZEHN VOR HALB"
    // LEDs = 20 - mm, direction = LEFT (additive towards :20)
    remainder = 20 - mm;
    if (remainder > 4) remainder = 0;  // Safety
    isLeftDir = true;
    result.direction = LEFT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 10c: SPECIAL CASE VIERTEL at :15-:19 (including fallback VIERTEL [next_hour]) =====
  bool hasViertel = false;
  for (uint8_t i = 0; i < wordCount; i++) {
    if (wordEquals(words[i], "VIERTEL")) {
      hasViertel = true;
      break;
    }
  }

  if (hasViertel && !hasHalb && mm >= 15 && mm <= 19) {
    // :15-:19 VIERTEL (either "VIERTEL NACH [h]" or fallback "VIERTEL [next_hour]")
    // Count minutes after :15
    remainder = mm - 15;  // 0, 1, 2, 3, 4 for :15-:19
    isLeftDir = true;
    result.direction = LEFT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  if (hasHalb && hasVor) {
    // PRIORITY 10: HALB + VOR → remainder = mm % 5, direction = right
    remainder = mm % 5;
    isLeftDir = false;
    result.direction = RIGHT;
    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== PRIORITY 11: VOR (no HALB) =====
  if (hasVor && !hasHalb) {
    bool dummy;
    uint8_t target = getTargetMinute(words, wordCount, dummy);

    // Check for FÜNF word
    bool hasFunf = false;
    for (uint8_t i = 0; i < wordCount; i++) {
      if (wordEquals(words[i], "FuNF")) hasFunf = true;
    }

    // ALL VOR (without HALB): Calculate distance to target minute
    // If mm < target: still X minutes away → LEFT direction (additive)
    // If mm >= target: already X minutes past → RIGHT direction (subtractive)
    if (mm < target) {
      // Haven't reached target yet: minutes remaining
      remainder = target - mm;
      isLeftDir = true;
      result.direction = LEFT;
    } else {
      // Already past target: minutes since target
      remainder = mm - target;
      isLeftDir = false;
      result.direction = RIGHT;
    }

    // Safety: cap at 4 LEDs
    if (remainder > 4) remainder = 0;

    result.count = remainder;
    result.hex = ledCountToHex(remainder, isLeftDir);
    return result;
  }

  // ===== DEFAULT: 0 LEDs =====
  result.count = 0;
  result.hex = 0x00;
  return result;
}
