/**
 * CharGraph TimeLogic - Black Box Test Configuration
 *
 * Define test patterns and time windows here
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// TEST PATTERNS
// ============================================================================
// Each pattern must be exactly 110 characters

const char testpatterns[][111] = {
  // Pattern 1: Standard Pattern
  "ESGISTWZEHNFÜNFVIERTELVORNACHBALDHALBKZWEINSYWAYOLOVDCQZEHNEUNACHTELFÜNFZWÖLFSECHSIEBENGEDREIVIERTMDCPAUSEUHRW",

  // Pattern 2: DREIVIERTEL Pattern
  "ESUISTKFASTBALDKURZEHNFÜNFZEITVORDREIVIERTELNACHRWDHALBHSECHSVIERKELFZWÖLFÜNFZEHNEUNACHTDREINSIEBENMZWEIEHUHRQ",

  // Add more patterns here as needed
  // "PATTERN3...",
  // "PATTERN4...",
};

// Number of test patterns
const uint8_t numPatterns = 2;

// ============================================================================
// TEST TIME WINDOW
// ============================================================================

uint8_t hourStart = 23;      // Start hour (0-23)
uint8_t hourEnd = 0;         // End hour (0-23, can wrap around)
uint8_t minuteStart = 23;    // Start minute (0-59)
uint8_t minuteEnd = 27;      // End minute (0-59)
uint8_t hops = 1;            // Minute increments (1 = every minute, 5 = every 5 minutes, etc.)

// ============================================================================
// SERIAL OUTPUT CONFIGURATION
// ============================================================================

#define SERIAL_BAUD 115200

#endif // CONFIG_H
