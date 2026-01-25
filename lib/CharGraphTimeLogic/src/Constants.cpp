/**
 * CharGraph Time Logic - Constants Implementation
 *
 * All string constants stored in PROGMEM
 */

#include "Constants.h"

// ============================================================================
// HOUR WORDS
// ============================================================================

const char HOUR_ZERO[] PROGMEM   = "ZWoLF";
const char HOUR_ONE[] PROGMEM    = "EINS";
const char HOUR_TWO[] PROGMEM    = "ZWEI";
const char HOUR_THREE[] PROGMEM  = "DREI";
const char HOUR_FOUR[] PROGMEM   = "VIER";
const char HOUR_FIVE[] PROGMEM   = "FuNF";
const char HOUR_SIX[] PROGMEM    = "SECHS";
const char HOUR_SEVEN[] PROGMEM  = "SIEBEN";
const char HOUR_EIGHT[] PROGMEM  = "ACHT";
const char HOUR_NINE[] PROGMEM   = "NEUN";
const char HOUR_TEN[] PROGMEM    = "ZEHN";
const char HOUR_ELEVEN[] PROGMEM = "ELF";

const char* const HOURS[12] PROGMEM = {
  HOUR_ONE,
  HOUR_TWO,
  HOUR_THREE,
  HOUR_FOUR,
  HOUR_FIVE,
  HOUR_SIX,
  HOUR_SEVEN,
  HOUR_EIGHT,
  HOUR_NINE,
  HOUR_TEN,
  HOUR_ELEVEN,
  HOUR_ZERO
};

// ============================================================================
// MANDATORY/OPTIONAL WORDS
// ============================================================================

const char ES[] PROGMEM      = "ES";
const char IST[] PROGMEM     = "IST";
const char HALB[] PROGMEM    = "HALB";
const char VIERTEL[] PROGMEM = "VIERTEL";
const char VOR[] PROGMEM     = "VOR";
const char NACH[] PROGMEM    = "NACH";
const char UHR[] PROGMEM     = "UHR";
const char EIN[] PROGMEM     = "EIN";

// ============================================================================
// OPTIONAL MODIFIERS
// ============================================================================

const char KURZ[] PROGMEM        = "KURZ";
const char BALD[] PROGMEM        = "BALD";
const char FAST[] PROGMEM        = "FAST";
const char ZWANZIG[] PROGMEM     = "ZWANZIG";
const char DREIVIERTEL[] PROGMEM = "DREIVIERTEL";
const char NACHT[] PROGMEM       = "NACHT";
const char WIR[] PROGMEM         = "WIR";
const char HABEN[] PROGMEM       = "HABEN";

// ============================================================================
// MINUTE WORDS
// ============================================================================

const char FUENF[] PROGMEM = "FuNF";
const char ZEHN[] PROGMEM  = "ZEHN";
const char EINS[] PROGMEM  = "EINS";

// ============================================================================
// LED DIRECTION STRINGS
// ============================================================================

const char LEFT[] PROGMEM  = "left";
const char RIGHT[] PROGMEM = "right";

// ============================================================================
// ERROR MESSAGES
// ============================================================================

const char ERR_NO_WORDS[] PROGMEM         = "No words";
const char ERR_WORD_NOT_FOUND[] PROGMEM   = "Word not found";
const char ERR_NO_GAP[] PROGMEM           = "No gap between words";
const char ERR_NO_ES[] PROGMEM            = "ES missing";
const char ERR_NO_IST[] PROGMEM           = "IST missing";
const char ERR_NO_HALB[] PROGMEM          = "HALB missing";
const char ERR_NO_WIR[] PROGMEM           = "WIR missing";
const char ERR_NO_HABEN[] PROGMEM         = "HABEN missing";
const char ERR_NO_GAP_ES_IST[] PROGMEM    = "No gap between ES and IST";
const char ERR_NO_GAP_WIR_HABEN[] PROGMEM = "No gap between WIR and HABEN";
const char ERR_UHR_NOT_LAST[] PROGMEM     = "UHR must be last word";

// ============================================================================
// MANDATORY WORDS ERROR MESSAGES
// ============================================================================

const char ERR_NO_FUENF[] PROGMEM   = "FÜNF missing";
const char ERR_NO_ZEHN[] PROGMEM    = "ZEHN missing";
const char ERR_NO_VIERTEL[] PROGMEM = "VIERTEL missing";
const char ERR_NO_VOR[] PROGMEM     = "VOR missing";
const char ERR_NO_NACH[] PROGMEM    = "NACH missing";
const char ERR_NO_GAP_VIERTEL_VOR[] PROGMEM  = "No gap between VIERTEL and VOR";
const char ERR_NO_GAP_VIERTEL_NACH[] PROGMEM = "No gap between VIERTEL and NACH";
const char ERR_NO_VIERTEL_SEQUENCE[] PROGMEM = "Cannot display :45 (VIERTEL before VOR required or DREIVIERTEL missing)";

// ============================================================================
// OPTIONAL WORDS
// ============================================================================

const char ZEIT[] PROGMEM  = "ZEIT";
const char ALARM[] PROGMEM = "ALARM";
const char PAUSE[] PROGMEM = "PAUSE";
const char RWD[] PROGMEM   = "RWD";

// ============================================================================
// OPTIONAL WORDS WARNING MESSAGES
// ============================================================================

const char WARN_NO_GAP_NACHT[] PROGMEM = "INFO: No gap between IST and NACHT";
const char WARN_NO_GAP_ZEIT[] PROGMEM  = "INFO: No gap between IST and ZEIT";
const char WARN_NO_GAP_ALARM[] PROGMEM = "INFO: No gap between IST and ALARM";
const char WARN_NO_GAP_PAUSE[] PROGMEM = "INFO: No gap between IST and PAUSE";
const char WARN_NO_GAP_RWD[] PROGMEM   = "INFO: No gap between IST and RWD";

// ============================================================================
// HELPER: Compare PROGMEM string with C-string
// ============================================================================

bool wordEquals(const char* word_progmem, const char* cstr) {
  if (!word_progmem || !cstr) return false;
  char buf[12];  // Max 11 chars (DREIVIERTEL) + null terminator
  strcpy_P(buf, word_progmem);
  return strcmp(buf, cstr) == 0;
}

// ============================================================================
// HELPER: Get hour word from PROGMEM array
// ============================================================================

const char* getHourWord(uint8_t h12) {
  if (h12 <= 0) h12 = 12;
  else if (h12 > 12) h12 -= 12;

  // Index into HOURS array (0-11 for h12 1-12)
  return (const char*)pgm_read_ptr(&HOURS[h12 - 1]);
}
