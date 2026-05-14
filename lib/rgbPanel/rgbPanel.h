#ifndef _RGBPANEL_H_
  #define _RGBPANEL_H_
  #include <FastLED.h>
  #include <defines.inc>
  #include <CharGraphTimeLogic.h>
  // ════════════════════════════════════════════════════════════════
  // HARDWARE DEFINITIONEN
  // ════════════════════════════════════════════════════════════════
  #define LED_PIN D6
  #define NUM_LEDS 121
  #define COLS 11
  #define ROWS 11
  // Charsoap belegt nur die ersten (ROWS-1) Zeilen (die letzte Zeile sind die
  // Minuten-LEDs), also 10*11 = 110 sichtbare Buchstaben.
  #define CHARSOAP_LEN (COLS * (ROWS - 1))

  #define MAXWORDS 3 //Anzahl Spezialwörter
  #define SPECIAL_WORD_LENGTH (COLS+2)
  extern CRGB normalColor;
  extern CRGB specialColor;
  extern CRGB leds[NUM_LEDS];
  extern uint8_t brightness;

  extern const int hourpattern[][2];
  extern int MINUTE_LEDS[4];
  extern char testPattern[];
  extern bool customCharsoap;
  extern char charsoap[(COLS * ROWS * 2)+1];
  extern const char DEFAULT_CHARSOAP[];
  extern char SPECIAL_WORD[MAXWORDS][SPECIAL_WORD_LENGTH];  // MAXWORDS = 3

  extern void showLEDs();
  extern int bridgeLED(int pos);
  extern void rgbTest();
  extern void checkPattern();
  extern int findWord(const char* word, int occurrence = 0, bool searchBackward = false);
  extern int setWord(const char* word, CRGB color, int occurrence = 0, bool searchBackward = false);
  extern void getLedsFromPosition(int startPos, int length, int* ledArray);
  extern void displayTime(int hours, int minutes);
  extern void displayTimeWithSpecial(int hours, int minutes);
  extern int setWordOnlyIfFree(const char* word, CRGB color, int occurrence = 0, bool searchBackward = false);
  extern void showSpecialWordThenTime(int hours, int minutes);
  extern void fadeOutAll(uint8_t steps, uint16_t stepDelayMs);
  extern void fadeInCurrentFrame(uint8_t targetBrightness, uint8_t steps, uint16_t stepDelayMs);
#endif
