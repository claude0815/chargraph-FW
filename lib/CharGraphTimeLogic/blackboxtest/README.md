# CharGraph TimeLogic - Black Box Test Suite

Automated testing framework for the CharGraphTimeLogic library.

## Overview

The Black Box Test Suite provides a standardized way to test the CharGraph time-to-words conversion logic across multiple patterns and time windows.

## Files

### blackboxtest.ino
Main test application. Loads patterns from `config.h`, runs tests, and outputs results to serial.

**Features:**
- Tests multiple patterns sequentially
- Configurable time window and minute increments
- Table-formatted output compatible with markdown
- Test statistics and pass/fail summary

### config.h
Configuration file for test parameters.

**Parameters:**

```cpp
// Test patterns (each must be exactly 110 characters)
const char testpatterns[][111] = {
  "PATTERN1_110_CHARS",
  "PATTERN2_110_CHARS",
  // Add more patterns here
};

// Number of patterns to test
const uint8_t numPatterns = 2;

// Time window
uint8_t hourStart = 23;      // Start hour (0-23)
uint8_t hourEnd = 0;         // End hour (can wrap around midnight)
uint8_t minuteStart = 23;    // Start minute (0-59)
uint8_t minuteEnd = 27;      // End minute (0-59)
uint8_t hops = 1;            // Minute increments (1=every min, 5=every 5 min, etc.)
```

## Usage

### Setup

1. Place `blackboxtest.ino` and `config.h` in your Arduino project folder
2. Ensure CharGraphTimeLogic library is properly installed
3. Modify patterns in `config.h` as needed

### Configuration

#### Adding Test Patterns

In `config.h`:
```cpp
const char testpatterns[][111] = {
  "ESGISTWZEHNFÜNFVIERTELVORNACHBALDHALBKZWEINSYWAYOLOVDCQZEHNEUNACHTELFÜNFZWÖLFSECHSIEBENGEDREIVIERTMDCPAUSEUHRW",  // Pattern 1
  "ESUISTKFASTBALDKURZEHNFÜNFZEITVORDREIVIERTELNACHRWDHALBHSECHSVIERKELFZWÖLFÜNFZEHNEUNACHTDREINSIEBENMZWEIEHUHRQ",  // Pattern 2
  // "PATTERN3...",
};
const uint8_t numPatterns = 2;
```

#### Changing Time Window

Default (Standard Black Box Test):
```cpp
uint8_t hourStart = 23;      // 23:23
uint8_t hourEnd = 0;         // to 00:27
uint8_t minuteStart = 23;
uint8_t minuteEnd = 27;
uint8_t hops = 1;            // Every minute
```

For testing just 1 hour:
```cpp
uint8_t hourStart = 14;
uint8_t hourEnd = 14;
uint8_t minuteStart = 0;
uint8_t minuteEnd = 59;
uint8_t hops = 1;
```

For sparse sampling (every 5 minutes):
```cpp
uint8_t hops = 5;
```

### Running Tests

1. Connect Arduino to computer via USB
2. Open Arduino IDE
3. Load `blackboxtest.ino`
4. Select correct board and port
5. Upload sketch
6. Open Serial Monitor (baud rate: 115200)
7. Tests will run automatically and output results

### Output Format

```
========================================
Pattern 1
========================================
Pattern: ESGISTWZEHNFÜNF...

Uhrzeit | Text | MinutenLeds | Links | Rechts
--------|------|-------------|-------|-------
23:23 | ES IST ZEHN VOR HALB ZWÖLF | *** | L | R
23:24 | ES IST ZEHN VOR HALB ZWÖLF | **** | L | R
...

========================================
TEST SUMMARY
========================================
Total Tests: 130
Passed: 130
Failed: 0

✓ ALL TESTS PASSED
```

## Test Statistics

After each pattern, a summary is printed:
- **Total Tests**: Number of minutes tested
- **Passed**: Successful word generation
- **Failed**: Errors or invalid results

## LED Output Format

In the serial output:
- `*` = One LED
- `****` = Four LEDs
- `L` = LED direction LEFT (additive)
- `R` = LED direction RIGHT (subtractive)

## Standard Time Windows

### Black Box Test (Default)
- **Start:** 23:23
- **End:** 00:27
- **Duration:** 65 minutes
- **Reason:** Tests full hour transition and critical minutes

### Full Hour Test
- **Start:** HH:00
- **End:** HH:59
- **Duration:** 60 minutes

### Sparse Test (Debug)
- **Start:** 23:23
- **End:** 00:27
- **Hops:** 5 minutes
- **Duration:** 13 tests (instead of 65)

## Troubleshooting

### Serial Output Garbled
- Check baud rate: Should be 115200
- Verify USB cable connection
- Try different USB port

### Pattern Not Found
- Verify pattern length is exactly 110 characters
- Check for accented characters (ä, ö, ü)
- Ensure pattern is uppercase

### Memory Issues
- Reduce number of patterns in config.h
- Increase hops value (test fewer minutes)
- Test on Arduino with more SRAM (Mega)

## Design Goals

✓ Simple, intuitive configuration
✓ Consistent time window (23:23-00:27 by default)
✓ Markdown-compatible output
✓ Deterministic, repeatable results
✓ Works on standard Arduino boards

## License

Part of CharGraph TimeLogic library. See main LICENSE for details.
