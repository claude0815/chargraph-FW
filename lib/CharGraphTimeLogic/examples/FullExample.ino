/**
 * CharGraph Time Logic - Full Example with Time Sync
 *
 * Complete example showing:
 * - Pattern validation
 * - Real-time conversion
 * - LED output
 * - Debug information
 *
 * Tested on: WEMOS D1 Mini (ESP8266)
 *
 * Hardware:
 * - ESP8266 / WEMOS D1 Mini
 * - USB Serial connection
 * - Optional: DS3231 RTC module (I2C)
 *
 * Instructions:
 * 1. Install library
 * 2. Configure pattern below
 * 3. Upload and monitor serial output
 */

#include <CharGraphTimeLogic.h>

// Define this to enable debug output
// #define CHARGRAPH_DEBUG

// 110-character pattern - MUST be exactly 110 characters
// Format: 10 rows of 11 characters each
// Row 1: ESIST-FUENF... (E S I S T - F Ü E N F = 11 chars)
// Row 2-10: similar
const char PATTERN[] PROGMEM =
  "ESIST-FUENFZEHNVIERTELVOR"
  "NACHABFASTHALBALDZWEI----"
  "DREEINSIEBENEFL-ZWOELF"
  "FUENF-SECHSVIER-ZEHNEUNACHT"
  "-------UHR";  // Total: 110 characters

// Current time (simulated or from RTC)
uint8_t currentHour = 14;
uint8_t currentMinute = 30;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  CharGraph Time Logic - Full Example   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();

  // Verify pattern length
  size_t patternLen = strlen_P(PATTERN);
  Serial.print("Pattern length: ");
  Serial.println(patternLen);

  if (patternLen == 110) {
    Serial.println("✓ Pattern length is correct (110 chars)");
  } else {
    Serial.print("✗ Pattern length ERROR! Expected 110, got ");
    Serial.println(patternLen);
  }

  Serial.println();
  Serial.println("Starting time simulation...");
  Serial.println();
}

void loop() {
  // Convert time to words
  CharGraphTimeWords result;

  if (getCharGraphWords(PATTERN, currentHour, currentMinute, result)) {
    // ===== Display current time =====
    displayTime(currentHour, currentMinute);

    // ===== Display words =====
    Serial.print("Words: ");
    for (uint8_t i = 0; i < result.wordCount; i++) {
      Serial.print((const __FlashStringHelper*) result.words[i]);
      if (i < result.wordCount - 1) Serial.print(" ");
    }
    Serial.println();

    // ===== Display LED information =====
    Serial.print("LED: ");
    Serial.print(result.ledCount);
    Serial.print(" × ");
    Serial.print((const __FlashStringHelper*) result.ledDirection);
    Serial.print(" = 0x");
    if (result.ledHex < 0x10) Serial.print("0");
    Serial.println(result.ledHex, HEX);

    // ===== Display binary representation =====
    Serial.print("Bits: ");
    for (int8_t i = 3; i >= 0; i--) {
      Serial.print((result.ledHex >> i) & 1);
    }
    Serial.println();

    Serial.println();
  } else {
    Serial.print("ERROR at ");
    if (currentHour < 10) Serial.print("0");
    Serial.print(currentHour);
    Serial.print(":");
    if (currentMinute < 10) Serial.print("0");
    Serial.println(currentMinute);
    Serial.println("Pattern validation failed!");
    Serial.println();
  }

  // ===== Advance time =====
  currentMinute += 5;
  if (currentMinute >= 60) {
    currentMinute = 0;
    currentHour += 1;
    if (currentHour >= 24) {
      currentHour = 0;
      Serial.println("═══════════════════════════════════════");
      Serial.println("            Daily cycle complete");
      Serial.println("═══════════════════════════════════════");
      Serial.println();
    }
  }

  delay(1000);  // 1 second per step (demonstrates all 60 minutes quickly)
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void displayTime(uint8_t hour, uint8_t minute) {
  Serial.print("Time: ");
  if (hour < 10) Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.print(minute);
  Serial.print("  →  ");
}

// ============================================================================
// OPTIONAL: RTC INTEGRATION
// ============================================================================

// If you have a DS3231 RTC module, uncomment and modify this section:
/*
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  // Set time (only on first run)
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void updateTimeFromRTC() {
  DateTime now = rtc.now();
  currentHour = now.hour();
  currentMinute = now.minute();
}
*/

// ============================================================================
// OPTIONAL: WIFI TIME SYNC (NTP)
// ============================================================================

// If you want to sync time from internet, add WiFi + time library:
/*
#include <time.h>

void setupWiFi() {
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Sync time from NTP
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Time synced");
}

void updateTimeFromNTP() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  currentHour = timeinfo->tm_hour;
  currentMinute = timeinfo->tm_min;
}
*/
