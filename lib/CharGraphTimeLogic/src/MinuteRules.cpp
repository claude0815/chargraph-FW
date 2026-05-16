/**
 * CharGraph Time Logic - Minute Rules Implementation
 *
 * All 24 rules for minutes :00-:59
 */

#include "MinuteRules.h"
#include "Constants.h"
#include <cstring>

// ============================================================================
// INDIVIDUAL RULE HANDLERS
// ============================================================================

// :00 - Full hour (with or without UHR)
static uint8_t rule_00(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasUhrAtEnd) {
    // Check if hour word starts with "EI" (EINS in PROGMEM)
    // Must use pgm_read_byte() to safely read from PROGMEM
    char first = pgm_read_byte(ctx.hourWord);
    char second = pgm_read_byte(ctx.hourWord + 1);
    outWords[0] = (first == 'E' && second == 'I') ? EIN : ctx.hourWord;
    outWords[1] = UHR;
    return 2;
  }
  outWords[0] = ctx.hourWord;
  return 1;
}

// :01-:02 - KURZ NACH or NACH
static uint8_t rule_01_02(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = NACH;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  outWords[0] = NACH;
  outWords[1] = ctx.hourWord;
  return 2;
}

// :03-:04 - NACH (oder KURZ NACH, wenn KURZ im Pattern)
static uint8_t rule_03_04(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz && ctx.fallbackLevel == 0) {
    outWords[0] = KURZ;
    outWords[1] = NACH;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  outWords[0] = NACH;
  outWords[1] = ctx.hourWord;
  return 2;
}

// :05-:09 - FÜNF NACH (oder FAST ZEHN NACH bei :08/:09, wenn FAST im Pattern)
static uint8_t rule_05_09(const RuleContext& ctx, const char** outWords) {
  if ((ctx.mm == 8 || ctx.mm == 9) && ctx.hasFast && ctx.fallbackLevel == 0) {
    outWords[0] = FAST;
    outWords[1] = ZEHN;
    outWords[2] = NACH;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  outWords[0] = FUENF;
  outWords[1] = NACH;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :10-:14 - ZEHN NACH (oder FAST VIERTEL NACH bei :13/:14, wenn FAST im Pattern)
static uint8_t rule_10_14(const RuleContext& ctx, const char** outWords) {
  if ((ctx.mm == 13 || ctx.mm == 14) && ctx.hasFast && ctx.fallbackLevel == 0) {
    outWords[0] = FAST;
    outWords[1] = VIERTEL;
    outWords[2] = NACH;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  outWords[0] = ZEHN;
  outWords[1] = NACH;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :15-:19 - VIERTEL NACH (with fallback support)
static uint8_t rule_15_19(const RuleContext& ctx, const char** outWords) {
  if (ctx.fallbackLevel == 0) {
    // Primary: VIERTEL NACH [h]
    // BUT: Check if NACH comes BEFORE VIERTEL in pattern
    // If so, VIERTEL NACH is not sequentially possible - fallback to VIERTEL [h+1]
    int16_t nach_pos = -1;
    int16_t viertel_pos = -1;

    if (ctx.gridStr) {
      // Find positions in pattern
      char nach_buf[5];  // NACH = 4 chars + null terminator
      strcpy_P(nach_buf, NACH);
      const char* nach_ptr = strstr(ctx.gridStr, nach_buf);
      if (nach_ptr) nach_pos = nach_ptr - ctx.gridStr;

      char viertel_buf[8];  // VIERTEL = 7 chars + null terminator
      strcpy_P(viertel_buf, VIERTEL);
      const char* viertel_ptr = strstr(ctx.gridStr, viertel_buf);
      if (viertel_ptr) viertel_pos = viertel_ptr - ctx.gridStr;
    }

    // If NACH comes BEFORE VIERTEL, use next hour instead
    if (nach_pos != -1 && viertel_pos != -1 && nach_pos < viertel_pos) {
      // Fallback: VIERTEL [next_hour]
      uint8_t next_h12 = (ctx.h12 % 12) + 1;
      const char* next_hour_word = getHourWord(next_h12);
      outWords[0] = VIERTEL;
      outWords[1] = next_hour_word;
      return 2;
    } else {
      // Normal case: VIERTEL NACH [h]
      outWords[0] = VIERTEL;
      outWords[1] = NACH;
      outWords[2] = ctx.hourWord;
      return 3;
    }
  }
  else if (ctx.fallbackLevel == 1) {
    // Secondary Fallback: VIERTEL [h+1] (when NACH not usable)
    uint8_t next_h12 = (ctx.h12 % 12) + 1;
    const char* next_hour_word = getHourWord(next_h12);
    outWords[0] = VIERTEL;
    outWords[1] = next_hour_word;
    return 2;
  }

  // Tertiary fallback (should not reach here)
  else if (ctx.mm >= 16 && ctx.mm <= 19) {
    // :16-:19 Fallback: ZEHN VOR HALB [h+1]
    uint8_t next_h12 = (ctx.h12 % 12) + 1;
    const char* next_hour_word = getHourWord(next_h12);
    outWords[0] = ZEHN;
    outWords[1] = VOR;
    outWords[2] = HALB;
    outWords[3] = next_hour_word;
    return 4;
  }

  // Default (should not be reached)
  return 0;
}

// :20-:24 - ZWANZIG NACH or ZEHN VOR HALB (with fallback support)
static uint8_t rule_20_24(const RuleContext& ctx, const char** outWords) {
  if (ctx.fallbackLevel == 0 && ctx.hasZwanzig) {
    // Primary: ZWANZIG NACH [h] - uses current hour
    outWords[0] = ZWANZIG;
    outWords[1] = NACH;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  // Fallback: ZEHN VOR HALB [h+1] - must use NEXT hour
  outWords[0] = ZEHN;
  outWords[1] = VOR;
  outWords[2] = HALB;
  outWords[3] = getHourWord((ctx.h12 + 1) % 12);
  return 4;
}

// :25-:26 - FÜNF VOR HALB
static uint8_t rule_25_26(const RuleContext& ctx, const char** outWords) {
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :27 - BALD HALB or FÜNF VOR HALB
static uint8_t rule_27(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = HALB;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :28 - KURZ VOR HALB > BALD HALB
static uint8_t rule_28(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = VOR;
    outWords[2] = HALB;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = HALB;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :29 - FAST > KURZ > BALD
static uint8_t rule_29(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasFast) {
    outWords[0] = FAST;
    outWords[1] = HALB;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = VOR;
    outWords[2] = HALB;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = HALB;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :30 - HALB
static uint8_t rule_30(const RuleContext& ctx, const char** outWords) {
  outWords[0] = HALB;
  outWords[1] = ctx.hourWord;
  return 2;
}

// :31-:32 - KURZ NACH HALB or NACH HALB
static uint8_t rule_31_32(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = NACH;
    outWords[2] = HALB;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  outWords[0] = NACH;
  outWords[1] = HALB;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :33-:34 - NACH HALB (oder KURZ NACH HALB, wenn KURZ im Pattern)
static uint8_t rule_33_34(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz && ctx.fallbackLevel == 0) {
    outWords[0] = KURZ;
    outWords[1] = NACH;
    outWords[2] = HALB;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  outWords[0] = NACH;
  outWords[1] = HALB;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :35-:39 - FÜNF NACH HALB
static uint8_t rule_35_39(const RuleContext& ctx, const char** outWords) {
  outWords[0] = FUENF;
  outWords[1] = NACH;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :40-:44 - ZEHN NACH HALB / ZWANZIG VOR / FAST VIERTEL VOR (with fallback)
static uint8_t rule_40_44(const RuleContext& ctx, const char** outWords) {
  // Bei :43/:44 + FAST: "FAST VIERTEL VOR [h]" – naeher dran als ZWANZIG VOR
  if ((ctx.mm == 43 || ctx.mm == 44) && ctx.hasFast && ctx.fallbackLevel == 0) {
    outWords[0] = FAST;
    outWords[1] = VIERTEL;
    outWords[2] = VOR;
    outWords[3] = ctx.hourWord;
    return 4;
  }
  if (ctx.fallbackLevel == 0 && ctx.hasZwanzig) {
    // Primary: ZWANZIG VOR [h]
    outWords[0] = ZWANZIG;
    outWords[1] = VOR;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  // Fallback: ZEHN NACH HALB [h]
  outWords[0] = ZEHN;
  outWords[1] = NACH;
  outWords[2] = HALB;
  outWords[3] = ctx.hourWord;
  return 4;
}

// :45 - DREIVIERTEL or VIERTEL VOR (pattern validation ensures one is always possible)
static uint8_t rule_45(const RuleContext& ctx, const char** outWords) {
  // DREIVIERTEL always takes priority if present, REGARDLESS of fallback level
  // This is critical: DREIVIERTEL must be returned on ALL attempts
  if (ctx.hasDreiviertel) {
    outWords[0] = DREIVIERTEL;
    outWords[1] = ctx.hourWord;
    return 2;
  }

  // Fallback: VIERTEL VOR [h]
  // This is only reached if DREIVIERTEL is NOT in the pattern
  // Pattern validation guarantees that if DREIVIERTEL is missing,
  // then VIERTEL must come BEFORE VOR (enabling VIERTEL VOR sequence)
  outWords[0] = VIERTEL;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :46-:47 - VIERTEL VOR (with fallback support)
static uint8_t rule_46_47(const RuleContext& ctx, const char** outWords) {
  if (ctx.fallbackLevel == 0) {
    // Primary: VIERTEL VOR [h]
    outWords[0] = VIERTEL;
    outWords[1] = VOR;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  // Fallback: ZEHN VOR [h]
  outWords[0] = ZEHN;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :48-:49 - ZEHN VOR (target 50, LED from left/added)
static uint8_t rule_48_49(const RuleContext& ctx, const char** outWords) {
  outWords[0] = ZEHN;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :50-:52 - ZEHN VOR
static uint8_t rule_50_52(const RuleContext& ctx, const char** outWords) {
  outWords[0] = ZEHN;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :53-:54 - FÜNF VOR (with LEFT direction for clarity: 5+2=7, 5+1=6)
static uint8_t rule_53_54(const RuleContext& ctx, const char** outWords) {
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :55-:56 - FÜNF VOR
static uint8_t rule_55_56(const RuleContext& ctx, const char** outWords) {
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :57 - BALD [HOUR] or FÜNF VOR
static uint8_t rule_57(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = ctx.hourWord;
    return 2;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :58 - KURZ VOR [HOUR] > BALD
static uint8_t rule_58(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = VOR;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = ctx.hourWord;
    return 2;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// :59 - FAST [HOUR] > KURZ VOR > BALD
static uint8_t rule_59(const RuleContext& ctx, const char** outWords) {
  if (ctx.hasFast) {
    outWords[0] = FAST;
    outWords[1] = ctx.hourWord;
    return 2;
  }
  if (ctx.hasKurz) {
    outWords[0] = KURZ;
    outWords[1] = VOR;
    outWords[2] = ctx.hourWord;
    return 3;
  }
  if (ctx.hasBald) {
    outWords[0] = BALD;
    outWords[1] = ctx.hourWord;
    return 2;
  }
  outWords[0] = FUENF;
  outWords[1] = VOR;
  outWords[2] = ctx.hourWord;
  return 3;
}

// ============================================================================
// EXECUTE MINUTE RULE
// ============================================================================

uint8_t executeMinuteRule(uint8_t minute, const RuleContext& ctx, const char** outWords) {
  switch (minute) {
    case 0:
      return rule_00(ctx, outWords);
    case 1:
    case 2:
      return rule_01_02(ctx, outWords);
    case 3:
    case 4:
      return rule_03_04(ctx, outWords);
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      return rule_05_09(ctx, outWords);
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      return rule_10_14(ctx, outWords);
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
      return rule_15_19(ctx, outWords);
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
      return rule_20_24(ctx, outWords);
    case 25:
    case 26:
      return rule_25_26(ctx, outWords);
    case 27:
      return rule_27(ctx, outWords);
    case 28:
      return rule_28(ctx, outWords);
    case 29:
      return rule_29(ctx, outWords);
    case 30:
      return rule_30(ctx, outWords);
    case 31:
    case 32:
      return rule_31_32(ctx, outWords);
    case 33:
    case 34:
      return rule_33_34(ctx, outWords);
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
      return rule_35_39(ctx, outWords);
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
      return rule_40_44(ctx, outWords);
    case 45:
      return rule_45(ctx, outWords);
    case 46:
    case 47:
      return rule_46_47(ctx, outWords);
    case 48:
    case 49:
      return rule_48_49(ctx, outWords);
    case 50:
    case 51:
    case 52:
      return rule_50_52(ctx, outWords);
    case 53:
    case 54:
      return rule_53_54(ctx, outWords);
    case 55:
    case 56:
      return rule_55_56(ctx, outWords);
    case 57:
      return rule_57(ctx, outWords);
    case 58:
      return rule_58(ctx, outWords);
    case 59:
      return rule_59(ctx, outWords);
    default:
      outWords[0] = nullptr;
      return 0;
  }
}
