/**
 * CharGraph Time Logic - Constants
 *
 * All arrays stored in PROGMEM for ESP8266 memory efficiency
 * Port of lib/time-logic/constants.ts
 */

#ifndef CHARGRAPH_CONSTANTS_H
#define CHARGRAPH_CONSTANTS_H

#include <Arduino.h>

// ============================================================================
// GRID PARAMETERS
// ============================================================================

#define GRID_SIZE 110    // 10 rows × 11 cols
#define GRID_ROWS 10
#define GRID_COLS 11

// ============================================================================
// HOUR WORDS (PROGMEM)
// ============================================================================

extern const char* const HOURS[12] PROGMEM;
extern const char HOUR_ZERO[] PROGMEM;
extern const char HOUR_ONE[] PROGMEM;
extern const char HOUR_TWO[] PROGMEM;
extern const char HOUR_THREE[] PROGMEM;
extern const char HOUR_FOUR[] PROGMEM;
extern const char HOUR_FIVE[] PROGMEM;
extern const char HOUR_SIX[] PROGMEM;
extern const char HOUR_SEVEN[] PROGMEM;
extern const char HOUR_EIGHT[] PROGMEM;
extern const char HOUR_NINE[] PROGMEM;
extern const char HOUR_TEN[] PROGMEM;
extern const char HOUR_ELEVEN[] PROGMEM;

// ============================================================================
// MANDATORY/OPTIONAL WORDS (PROGMEM)
// ============================================================================

extern const char ES[] PROGMEM;
extern const char IST[] PROGMEM;
extern const char HALB[] PROGMEM;
extern const char VIERTEL[] PROGMEM;
extern const char VOR[] PROGMEM;
extern const char NACH[] PROGMEM;
extern const char UHR[] PROGMEM;
extern const char EIN[] PROGMEM;

// ============================================================================
// OPTIONAL MODIFIERS (PROGMEM)
// ============================================================================

extern const char KURZ[] PROGMEM;
extern const char BALD[] PROGMEM;
extern const char FAST[] PROGMEM;
extern const char ZWANZIG[] PROGMEM;
extern const char DREIVIERTEL[] PROGMEM;
extern const char NACHT[] PROGMEM;
extern const char WIR[] PROGMEM;
extern const char HABEN[] PROGMEM;

// ============================================================================
// MINUTE WORDS (PROGMEM)
// ============================================================================

extern const char FUENF[] PROGMEM;
extern const char ZEHN[] PROGMEM;
extern const char EINS[] PROGMEM;

// ============================================================================
// LED DIRECTION STRINGS (PROGMEM)
// ============================================================================

extern const char LEFT[] PROGMEM;
extern const char RIGHT[] PROGMEM;

// ============================================================================
// LED BIT VALUES
// ============================================================================

#define LED1_LEFT  0x08  // Bit 3
#define LED2_LEFT  0x04  // Bit 2
#define LED2_RIGHT 0x02  // Bit 1
#define LED1_RIGHT 0x01  // Bit 0

// ============================================================================
// ERROR MESSAGES (PROGMEM)
// ============================================================================

extern const char ERR_NO_WORDS[] PROGMEM;
extern const char ERR_WORD_NOT_FOUND[] PROGMEM;
extern const char ERR_NO_GAP[] PROGMEM;
extern const char ERR_NO_ES[] PROGMEM;
extern const char ERR_NO_IST[] PROGMEM;
extern const char ERR_NO_HALB[] PROGMEM;
extern const char ERR_NO_WIR[] PROGMEM;
extern const char ERR_NO_HABEN[] PROGMEM;
extern const char ERR_NO_GAP_ES_IST[] PROGMEM;
extern const char ERR_NO_GAP_WIR_HABEN[] PROGMEM;
extern const char ERR_UHR_NOT_LAST[] PROGMEM;
extern const char ERR_NO_FUENF[] PROGMEM;
extern const char ERR_NO_ZEHN[] PROGMEM;
extern const char ERR_NO_VIERTEL[] PROGMEM;
extern const char ERR_NO_VOR[] PROGMEM;
extern const char ERR_NO_NACH[] PROGMEM;
extern const char ERR_NO_GAP_VIERTEL_VOR[] PROGMEM;
extern const char ERR_NO_GAP_VIERTEL_NACH[] PROGMEM;
extern const char ERR_NO_VIERTEL_SEQUENCE[] PROGMEM;

// ============================================================================
// OPTIONAL WORDS
// ============================================================================

extern const char ZEIT[] PROGMEM;
extern const char ALARM[] PROGMEM;
extern const char PAUSE[] PROGMEM;
extern const char RWD[] PROGMEM;

// ============================================================================
// OPTIONAL WORDS WARNING MESSAGES
// ============================================================================

extern const char WARN_NO_GAP_NACHT[] PROGMEM;
extern const char WARN_NO_GAP_ZEIT[] PROGMEM;
extern const char WARN_NO_GAP_ALARM[] PROGMEM;
extern const char WARN_NO_GAP_PAUSE[] PROGMEM;
extern const char WARN_NO_GAP_RWD[] PROGMEM;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

extern bool wordEquals(const char* word_progmem, const char* cstr);
extern const char* getHourWord(uint8_t h12);

#endif // CHARGRAPH_CONSTANTS_H
