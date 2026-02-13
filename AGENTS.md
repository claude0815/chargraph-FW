# AGENTS.md

## Project Overview

ESP8266 PlatformIO firmware for a German word clock (Wortuhr). An 11×11 WS2812B LED matrix (121 LEDs) displays time as illuminated German words. WiFi-controllable with AP mode for configuration and Station mode for NTP time sync.

## Build Commands

```bash
# Build (default environment: esp8266clone)
pio run

# Build for specific environment
pio run -e esp8266clone
pio run -e d1_mini

# Build and upload
pio run --target upload

# Serial monitor (115200 baud)
pio device monitor

# Clean build
pio run --target clean

# Update libraries
pio lib update
```

### Pre-build Script

`html_to_header.py` runs automatically before each build. It converts `html/index.html` into gzip-compressed C header `include/html.h`. Do not edit `html.h` directly — edit `html/index.html` instead.

## Testing

There is no unit test framework. Testing is done on-device:

```bash
# Run black box test (upload to device, output on serial)
# Edit lib/CharGraphTimeLogic/blackboxtest/config.h to configure:
#   - testpatterns[]: 110-char grid patterns
#   - hourStart/hourEnd, minuteStart/minuteEnd, hops
# Then upload the blackboxtest.ino sketch

# Enable word test mode via build flag in platformio.ini:
#   -DTEST_ALLTHETIME=true
```

### Debug Flags (platformio.ini build_flags)

- `DEBUG_SOURCE=true` — Enable serial debug output
- `USE_RTC=false` — Disable RTC (DS1307) functionality
- `TEST_RGB_ONLY=false` — Run only RGB LED test on boot
- `TEST_ALLTHETIME=false` — Run word-finding test for all times
- `POWERLOSSDETECT=false` — Enable power loss detection
- `CHARGRAPH_DEBUG` — Enable CharGraphTimeLogic debug output

## Architecture

### File Organization

- `src/main.cpp` — All application logic (~2660 lines): setup/loop, web handlers, EEPROM, NTP, OTA, WiFi, charsoap management
- `include/common.inc` — Central include hub; imports all dependencies
- `include/defines.inc` — Hardware defines, debug macros, EEPROM address map
- `include/vars.inc` — Global variable definitions (also includes `html.h` at bottom)
- `include/extern.inc` — `extern` declarations for cross-module globals
- `include/charPattern.inc` — Default character pattern (PROGMEM), test words, MINUTE_LEDS
- `include/WordFinder.h` — Unified word search (PROGMEM + string, forward/backward)
- `lib/rgbPanel/` — LED matrix control, word finding/highlighting, fade effects
- `lib/CharGraphTimeLogic/` — Time-to-words conversion engine with validation
- `lib/PowerOnDetector/` — Boot reason analysis, power loss detection
- `lib/info/` — Serial connection status display

### Include System

`main.cpp` includes `common.inc`, which chains: `extern.inc` → `defines.inc` → `version.inc` → `vars.inc` → library headers. `vars.inc` includes `html.h` at bottom. `charPattern.inc` is included only by `rgbPanel.cpp`.

### EEPROM Layout

512 bytes total. All addresses are `#define`d in `defines.inc` with `ADDR_` prefix. Magic bytes (e.g., `0xAA`, `0xBB`) are used as "configured" flags. Always call `EEPROM.commit()` after writes.

## Code Style Guidelines

### Indentation and Formatting

- **Indentation**: Mixed — mostly 4 spaces; some files use 2-space indent inside `#ifndef` guards. Use 4 spaces for new code.
- **Brace style**: Allman style (opening brace on its own line) for function definitions; K&R style (opening brace on same line) for `if`/`for`/`while`/`switch`. Both appear in the codebase — follow the style of the surrounding code.
- **Line length**: No strict limit; keep reasonable (~120 chars).
- **Section headers**: Use decorated comment blocks:
  ```cpp
  // ════════════════════════════════════════════════════════════════
  // SECTION NAME
  // ════════════════════════════════════════════════════════════════
  ```

### Naming Conventions

- **Functions**: `camelCase` — `loadCharsoap()`, `setRunningFlag()`, `findWord()`, `displayTime()`
- **Global variables**: `camelCase` — `bootCounter`, `ntpEnabled`, `lastSyncTime`, `autoBrightnessMin`
- **Constants**: `UPPER_SNAKE_CASE` — `NUM_LEDS`, `MAX_BRIGHTNESS`, `ADDR_CHARSOAP`
- **Macros/defines**: `UPPER_SNAKE_CASE` — `DEBUG_PRINTLN`, `RUNNING_FLAG_MAGIC`
- **Structs**: `PascalCase` — `ValidationResult`, `RuleContext`, `CharGraphTimeWords`, `TestStats`
- **Struct members**: `camelCase` — `wordCount`, `ledHex`, `fallbackLevel`
- **Header guards**: `_NAME_H_` for legacy files, `NAME_H` for newer CharGraphTimeLogic files

### Types

- Prefer Arduino/fixed-width types: `uint8_t`, `uint16_t`, `uint32_t`, `int8_t`, `int16_t`
- Use `bool` for flags
- Use `CRGB` for LED colors (FastLED)
- Use `char[]` / `char*` for strings — `String` class is used sparingly (only `otaError`)
- Use `const char*` with `PROGMEM` for flash-stored constant strings
- Use `unsigned char` when doing byte-level operations (UTF-8 handling)

### Comments

- Comments are a mix of German and English. German is predominant in older code (`main.cpp`, `rgbPanel`, `PowerOnDetector`). English is used in newer `CharGraphTimeLogic` library.
- Use `//` for inline comments; `/** ... */` Javadoc-style for function documentation in CharGraphTimeLogic
- Commented-out code is common (debug prints, alternative patterns) — leave existing commented code in place

### Error Handling

- Functions return `-1` or `false` on failure, valid values on success
- `getCharGraphWords()` returns `int8_t`: positive = success, 0 or negative = error
- Pattern validation returns `ValidationResult` struct with `bool valid` and `const char* reason`
- Debug output via macros: `DEBUG_PRINT(x)`, `DEBUG_PRINTLN(x)`, `DEBUG_PRINTF(...)` — these compile to no-ops when `DEBUG_SOURCE` is false
- Use `yield()` in long loops to prevent watchdog resets
- Always check `EEPROM.commit()` after EEPROM writes

### PROGMEM Usage

- Store constant strings and large data in flash with `PROGMEM`
- Read PROGMEM strings with `pgm_read_byte()`, `strcpy_P()`, `pgm_read_ptr()`
- Pattern strings are 110 characters (11 cols × 10 rows, row 11 is minute LEDs)

### Include Patterns

Library headers include their own dependencies:
```cpp
#include <Arduino.h>       // Always first
#include <FastLED.h>       // Hardware libraries
#include <EEPROM.h>
#include <defines.inc>     // Project includes use angle brackets
#include <extern.inc>
#include "Types.h"         // Intra-library includes use quotes
```

### Web Server Handlers

Handlers in `main.cpp` follow this pattern:
```cpp
server.on("/endpoint", []() {
    // Parse server.arg("paramName")
    // Process request
    // EEPROM.write(...) + EEPROM.commit() for persistence
    server.send(200, "application/json", "{\"status\":\"ok\"}");
});
```

### Hardware Constraints

- ESP8266 has ~40KB usable RAM — avoid dynamic allocation, prefer stack buffers
- LED updates require interrupt protection: call `noInterrupts()` before `FastLED.show()`, `interrupts()` after
- LED index 2 is always off (photoresistor position)
- Zigzag LED mapping: even rows left-to-right, odd rows right-to-left
- Call `yield()` or `delay()` in loops to feed the watchdog and allow WiFi processing

### Key Domain Knowledge

- "charsoap" = the 110-character letter grid that defines which words can be displayed
- Umlauts Ä→a, Ö→o, Ü→u in charsoap (stored as lowercase: `FuNF` = FÜNF, `ZWoLF` = ZWÖLF)
- Words list format: dash-separated string `"ES-IST-HALB-UHR-..."`
- Time display advances hour at minute >= 20 (German convention: "zehn vor drei" at 2:50)
