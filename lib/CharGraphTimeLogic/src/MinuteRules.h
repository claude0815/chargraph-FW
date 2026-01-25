/**
 * CharGraph Time Logic - Minute Rules
 *
 * Port of lib/time-logic/minute-rules.ts
 * 24 rules for minutes :00-:59 with handler functions
 */

#ifndef CHARGRAPH_MINUTE_RULES_H
#define CHARGRAPH_MINUTE_RULES_H

#include "Types.h"

/**
 * Minute rule handler function type
 * Called with RuleContext and returns array of word pointers
 *
 * @param ctx Rule context with modifiers and grid
 * @param outWords Output array for result words (max 6 words)
 * @return Number of words in output
 */
typedef uint8_t (*MinuteRuleHandler)(const RuleContext& ctx, const char** outWords);

/**
 * Find and execute the minute rule for a given minute
 *
 * @param minute Minute value (0-59)
 * @param ctx Rule context with pattern and modifiers
 * @param outWords Output array for result words
 * @return Number of words returned by handler
 */
uint8_t executeMinuteRule(uint8_t minute, const RuleContext& ctx, const char** outWords);

#endif // CHARGRAPH_MINUTE_RULES_H
