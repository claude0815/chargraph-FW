/**
 * CharGraph Time Logic - LED Calculator
 *
 * Port of lib/time-logic/led-calculator.ts
 * Minute LED calculation with 11-tier priority system
 */

#ifndef CHARGRAPH_LED_CALCULATOR_H
#define CHARGRAPH_LED_CALCULATOR_H

#include "Types.h"

/**
 * Get target minute for NACH/VOR-based LED calculation
 *
 * Returns the minute boundary that the current time counts towards or from.
 * Examples:
 * - FÜNF NACH :05 → target :05
 * - ZEHN NACH :10 → target :10
 * - VIERTEL NACH :15 → target :15
 * - ZEHN VOR :50 → target :50 (for :40-:44)
 * - FÜNF VOR :55 → target :55 (for :50-:54)
 * - VIERTEL VOR :45 → target :45 (for :40-:44)
 *
 * @param words Array of word pointers
 * @param wordCount Number of words
 * @param isLeftDirection Returns true if additive (left/NACH), false if subtractive (right/VOR)
 * @return Target minute value (0-59)
 */
uint8_t getTargetMinute(
  const char* const* words,
  uint8_t wordCount,
  bool& isLeftDirection
);

/**
 * Convert LED count and direction to 4-bit hex value
 *
 * LEFT direction (additive):
 * - 0 LEDs → 0x00
 * - 1 LED  → 0x08 (bit 3)
 * - 2 LEDs → 0x0C (bits 3+2)
 * - 3 LEDs → 0x0E (bits 3+2+1)
 * - 4 LEDs → 0x0F (bits 3+2+1+0)
 *
 * RIGHT direction (subtractive):
 * - 0 LEDs → 0x00
 * - 1 LED  → 0x01 (bit 0)
 * - 2 LEDs → 0x03 (bits 0+1)
 * - 3 LEDs → 0x07 (bits 0+1+2)
 * - 4 LEDs → 0x0F (bits 0+1+2+3)
 *
 * @param count LED count (0-4)
 * @param isLeft true for left direction, false for right
 * @return 4-bit hex value (0x00-0x0F)
 */
uint8_t ledCountToHex(uint8_t count, bool isLeft);

/**
 * Calculate LED position and direction for given words and minute
 *
 * 11-Tier Priority System (from charMatrixV03.html):
 * 1. DREIVIERTEL → remainder = mm - 45, direction = left
 * 2. BALD/FAST + HALB → remainder = targetMinute - mm, direction = right
 * 3. BALD/FAST (no HALB, :57-59) → remainder = 60 - mm, direction = right
 * 4. KURZ VOR + HALB → remainder = targetMinute - mm, direction = right
 * 5. KURZ VOR (no HALB, :58) → remainder = 60 - mm, direction = right
 * 6. KURZ NACH (no HALB, :01-02) → remainder = mm - targetMinute, direction = left
 * 7. KURZ NACH + HALB → remainder = mm % 5, direction = left
 * 8. NACH (no HALB) → remainder = mm - targetMinute, direction = left
 * 9. HALB + NACH → remainder = mm % 5, direction = left
 * 10. HALB + VOR → remainder = mm % 5, direction = right
 * 11. VOR (no HALB) → remainder = |mm - targetMinute|, direction = right
 *
 * @param words Array of word pointers (PROGMEM strings)
 * @param wordCount Number of words
 * @param mm Current minute (0-59)
 * @return LEDInfo with count, direction, and hex value
 */
LEDInfo calculateLEDs(
  const char* const* words,
  uint8_t wordCount,
  uint8_t mm
);

#endif // CHARGRAPH_LED_CALCULATOR_H
