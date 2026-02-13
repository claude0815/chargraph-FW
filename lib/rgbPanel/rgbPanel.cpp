#include <Arduino.h>
#include <rgbPanel.h>

#include <charPattern.inc>
// ════════════════════════════════════════════════════════════════
// Optimiertes FastLED.show() mit Interrupt-Schutz
// ════════════════════════════════════════════════════════════════
CRGB normalColor  = CRGB::White;
CRGB specialColor = CRGB::Green;
CRGB leds[NUM_LEDS];
uint8_t brightness = 80;

char charsoap[(COLS * ROWS * 2) + 1] = {'\0'}; //Char * 2 to fit all in, cause UTF (maybe ÄÖÜ) 
bool customCharsoap = false;

// ── Konfiguration für Spezialanzeige ──
// MAXWORDS ist in defines.inc definiert
// Platzhalter - werden dynamisch aus EEPROM oder Defaults geladen (siehe loadSpecialWords() in main.cpp)
char SPECIAL_WORD[MAXWORDS][13] = {"NAJAMHNAVIN\0",  //max 11 Zeichen, \0 wird automatisch angefügt!
                                   "\0",
                                   "\0" };   //Text der ein- bzw. ausgeblendet werden soll
const uint16_t SPECIAL_HOLD_MS = 5000;       // Anzeigedauer des Spezialworts

const int hourpattern[][2] = {
    {46, 7},
    {102, 5}
};


void showLEDs()
{
  noInterrupts();
  FastLED.show();
  interrupts();
}

//eleminate bridged LEDS
int bridgeLED(int pos)
{
    #ifndef BRIDGE_LEDS_POS77
      return pos;
    #else
      //if(pos==77)
      //  return NUM_LEDS;
      //else
        return (pos >= 77 ? pos-1:pos);      
    #endif
}

// Sanftes Ausblenden des aktuellen Frames (komplette Matrix)
// OPTIMIERT: Weniger Schritte und kürzeres Delay für bessere WiFi-Performance
#define MAX_STEPS 200
void fadeOutAll(uint8_t steps = MAX_STEPS, uint16_t stepDelayMs = 15) {
  if(steps > MAX_STEPS)
    steps = MAX_STEPS;

  for (uint8_t i = 0; i < MAX_STEPS; i++) {
    fadeToBlackBy(leds, NUM_LEDS, MAX_STEPS / steps);
    showLEDs();
    yield();
    delay(stepDelayMs);
  }
  FastLED.clear();
  showLEDs();
}

// Sanftes Einblenden mittels globaler Helligkeit
// OPTIMIERT: Weniger Schritte und kürzeres Delay für bessere WiFi-Performance
void fadeInCurrentFrame(uint8_t targetBrightness, uint8_t steps = MAX_STEPS, uint16_t stepDelayMs = 15) {
  if(steps > MAX_STEPS)
    steps = MAX_STEPS;
  uint8_t saved = targetBrightness;
  FastLED.setBrightness(0);
  showLEDs();
  // Schrittweite so wählen, dass exakt targetBrightness erreicht wird
  for (uint8_t s = 1; s <= steps; s++)
  {
    uint8_t b = (uint16_t)s * saved / steps;
    FastLED.setBrightness(b);
    showLEDs();
    yield();
    delay(stepDelayMs);
  }
  FastLED.setBrightness(saved);
  showLEDs();
}

// Zeigt ein Wort (falls vorhanden) mit sanftem Fade-In, hält es und blendet es wieder aus
// OPTIMIERT: Schnellere Animation für bessere WiFi-Performance
bool showSpecialWordSequence(const char words[][SPECIAL_WORD_LENGTH], CRGB color, uint8_t steps = 20, uint16_t stepDelayMs = 15)
{

  int pos = findWord(words[0], 0);
  if (pos < 0) {
    // Wort nicht gefunden
    return false;
  }

  // Volle Matrix zunächst aus
  FastLED.clear();

  for(uint i = 0; i < MAXWORDS; i++ )
  {
    if(findWord(words[i], 0))
    {
      // Wort setzen
      setWord(words[i], color, 0);
    }
  }

  // Helligkeit schonend einblenden
  uint8_t savedBrightness = FastLED.getBrightness(); // optional; wenn nicht verfügbar, nimm die globale 'brightness'
  if (savedBrightness == 0) savedBrightness = 80;     // Fallback
  fadeInCurrentFrame(savedBrightness, steps, stepDelayMs);

  // Für die gewünschte Dauer halten
  unsigned long t0 = millis();
  while (millis() - t0 < SPECIAL_HOLD_MS) {
    // Optional leichte "Herzschlag"-Animation vermeiden → einfach halten
    yield();
    delay(10);
  }

  // Ausblenden
  fadeOutAll(steps, stepDelayMs);
  return true;
}

// Orchestriert: bisherigen Text ausblenden → Spezialwort zeigen → neue Zeit rendern (mit sanftem Einblenden)
void showSpecialWordThenTime(int hours, int minutes)
{
  // Aktuelle Helligkeit speichern (BEVOR wir sie auf 0 setzen!)
  uint8_t savedBrightness = FastLED.getBrightness();
  if (savedBrightness == 0) savedBrightness = map(brightness, 0, 80, 0, 204); // Fallback auf globale Variable (gemappt)

  // 1) Bisherigen Frame ausblenden
  fadeOutAll();

  // 2) Spezialwort-Sequenz (falls im Layout vorhanden), Farbe: specialColor
  showSpecialWordSequence(SPECIAL_WORD, specialColor);

  // 3) Bisherigen Frame ausblenden
  fadeOutAll();

  // 4) Neue Zeit zeichnen
  //FastLED.clear();

  FastLED.setBrightness(0);      // Helligkeit vor dem internen showLEDs der displayTime auf 0 setzen

  displayTime(hours, minutes);

  // 5) Neue Zeit sanft einblenden (falls displayTime viel setzt, ist der Effekt angenehm)
  fadeInCurrentFrame(savedBrightness);
}

// ════════════════════════════════════════════════════════════════
// FUNKTIONEN: RGB-TEST
// ════════════════════════════════════════════════════════════════
void rgbTest()
{
    DEBUG_PRINTLN("\n╔════════════════════════════════╗");
    DEBUG_PRINTLN("║   RGB LED TEST ROUTINE         ║");
    DEBUG_PRINTLN("╚════════════════════════════════╝\n");
    
    // Test 1: Erste LED
    DEBUG_PRINTLN("Test 1: Erste LED (Rot)");
    FastLED.clear();
    leds[bridgeLED(0)] = CRGB::Red;
    showLEDs();
    delay(1000);
    
    // Test 2: Letzte LED
    DEBUG_PRINTLN("Test 2: Letzte LED (Blau)");
    FastLED.clear();
    leds[bridgeLED(NUM_LEDS - 1)] = CRGB::Blue;
    showLEDs();
    delay(1000);
    
    // Test 3: Alle Rot
    DEBUG_PRINTLN("Test 3: Alle LEDs Rot");
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    showLEDs();
    delay(1000);
    
    // Test 4: Alle Grün
    DEBUG_PRINTLN("Test 4: Alle LEDs Grün");
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    showLEDs();
    delay(1000);
    
    // Test 5: Alle Blau
    DEBUG_PRINTLN("Test 5: Alle LEDs Blau");
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    showLEDs();
    delay(1000);
    
    // Test 6: Alle Weiß
    DEBUG_PRINTLN("Test 6: Alle LEDs Weiß");
    fill_solid(leds, NUM_LEDS, CRGB::White);
    showLEDs();
    delay(1000);
    
    // Test 7: Lauflicht
    DEBUG_PRINTLN("Test 7: Lauflicht");
    FastLED.clear();
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[bridgeLED(i)] = CRGB::Green;
        showLEDs();
        delay(100);
        leds[bridgeLED(i)] = CRGB::Black;
    }
    
    // Test 8: Regenbogen
    DEBUG_PRINTLN("Test 8: Regenbogen");
    for (int hue = 0; hue < 256; hue += 4) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[bridgeLED(i)] = CHSV((hue + i * 2) % 256, 255, 255);
        }
        showLEDs();
        delay(20);
    }
    
    // Test 9: Matrix Zeilen
    DEBUG_PRINTLN("Test 9: Matrix Zeilen");
    for (int row = 0; row < ROWS; row++) {
        FastLED.clear();
        for (int col = 0; col < COLS; col++) {
            int index;
            if (row % 2 == 0) {
                index = row * COLS + col;
            } else {
                index = row * COLS + (COLS - 1 - col);
            }
            leds[bridgeLED(index)] = CRGB::Blue;
        }
        showLEDs();
        delay(300);
    }
    
    // Test 10: Matrix Spalten
    DEBUG_PRINTLN("Test 10: Matrix Spalten");
    for (int col = 0; col < COLS; col++) {
        FastLED.clear();
        for (int row = 0; row < ROWS; row++) {
            int index;
            if (row % 2 == 0) {
                index = row * COLS + col;
            } else {
                index = row * COLS + (COLS - 1 - col);
            }
            leds[bridgeLED(index)] = CRGB::Orange;
        }
        showLEDs();
        delay(300);
    }
    
    //Test 11: Minuten-LEDs
    DEBUG_PRINTLN("Test 11: Minuten-LEDs (4 Eck-LEDs)");
    
    // Alle 4 nacheinander
    CRGB minuteColors[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow};
    for (int i = 0; i < 4; i++) {
        FastLED.clear();
        leds[bridgeLED(MINUTE_LEDS[i])] = minuteColors[i];
        DEBUG_PRINTF("  → Minuten-LED %d (Index %d)\n", i+1, MINUTE_LEDS[i]);
        showLEDs();
        delay(500);
    }

    FastLED.clear();
    showLEDs();
    
    DEBUG_PRINTLN("\n✓ RGB Test abgeschlossen!\n");
}


void getLedsFromPosition(int startPos, int length, int* ledArray)
{
    for (int i = 0; i < length; i++) {
        int pos = startPos + i;
        int row = pos / COLS;
        int col = pos % COLS;
        
        if (row % 2 == 0) {
            ledArray[i] = row * COLS + col;
        } else {
            ledArray[i] = row * COLS + (COLS - 1 - col);
        }
    }
}


int setWord(const char* word, CRGB color, int occurrence, bool searchBackward)
{
    int pos = findWord(word, occurrence, searchBackward);
    if (pos == -1)
    {
       return -1;
    }
    int length = strlen(word);
    int ledIndices[length];
    getLedsFromPosition(pos, length, ledIndices);

    for (int i = 0; i < length; i++)
    {
      leds[bridgeLED(ledIndices[i])] = color;
    }
    return pos;
}

// ════════════════════════════════════════════════════════════════
// FUNKTIONEN: LED-ANSTEUERUNG
// ════════════════════════════════════════════════════════════════
int findWord(const char* word, int occurrence, bool searchBackward)
{
  const int wordLen = strlen(word);
  const int soapLen = strlen(charsoap);
  //DEBUG_PRINTF("Search Word '%s' %s", word, searchBackward ? "(backward) " : "");

  if (searchBackward)
  {
    // Rückwärtssuche: vom Ende zum Anfang
    int foundCount = 0;
    for (int pos = soapLen - wordLen; pos >= 0; --pos)
    {
      bool match = true;
      for (int j = 0; j < wordLen; ++j)
      {
        unsigned char c1 = charsoap[pos + j];
        unsigned char c2 = word[j];

        //lowercase => FuNF => funf
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');

        if (c1 != c2)
        {
          match = false;
          break;
        }
      }

      if (match)
      {
        if (foundCount == occurrence)
        {
          //DEBUG_PRINTF(" found on pos %d (%d) ✓\n", pos, occurrence);
          return pos;
        }
        foundCount++;
      }
    }
    //DEBUG_PRINTF(" not found (%d) ❌\n", occurrence);
    return -1;
  }
  else
  {
    // Vorwärtssuche: vom Anfang zum Ende (original)
    int start = 0;
    for (int i = 0; i <= occurrence; ++i)
    {
      int found = -1;
      for (int pos = start; pos <= soapLen - wordLen; ++pos)
      {
        bool match = true;
        for (int j = 0; j < wordLen; ++j)
        {
          unsigned char c1 = charsoap[pos + j];
          unsigned char c2 = word[j];

          //lowercase => FuNF => funf
          if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
          if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');

          if (c1 != c2)
          {
            match = false;
            break;
          }
        }

        if (match)
        {
          found = pos;
          break;
        }
      }
      if (found == -1)
      {
        //DEBUG_PRINTF(" not found (%d) ❌\n", occurrence);
        return -1;     // dieses i-te Vorkommen existiert nicht
      }

      if (i == occurrence)
      {
        //DEBUG_PRINTF(" found on pos %d (%d) ✓\n", found, occurrence);
        return found;
      }
      start = found + 1;              // ab nächster Position weiter suchen
    }
  }
  return -1;
}

uint8_t testWords()
{
  #if (defined(TEST_ALLTHETIME) && TEST_ALLTHETIME == false)
    return 0;
  #endif

  DEBUG_PRINT("╔════════════════════════╗\n");
  DEBUG_PRINT("║   Teste Wörter ...     ║\n");
  DEBUG_PRINT("╚════════════════════════╝\n")
  DEBUG_PRINTLN(charsoap);
  if(strlen(charsoap) != (COLS * (ROWS-1)))
  {
    //return -1;
    DEBUG_PRINT("\nLänge stimmt nicht: ");
    DEBUG_PRINTLN(strlen(charsoap));
  }
  else
  {
    DEBUG_PRINT("\nLänge stimmt: ");
    DEBUG_PRINTLN(strlen(charsoap));
  }
  uint8_t currentHour   = 0;
  uint8_t currentMinute = 0;
  //CharGraphTimeWords result;
  while(true)
  {
    yield();
    currentMinute++;
    if(currentMinute == 60)
    {
      currentHour   += 1;
      currentMinute  = 0;
    }
    if(currentHour == 23 && currentMinute == 59)
    {
      DEBUG_PRINT  ("\n╔════════════════════════╗\n");
      DEBUG_PRINT  (  "║   Teste Wörter fertig. ║\n");
      DEBUG_PRINTLN(  "╚════════════════════════╝\n")
      delay(1500);
      return 0;
    } 

    DEBUG_PRINT("Zeit: '");
    if (currentHour < 10) DEBUG_PRINT("0");
    DEBUG_PRINT(currentHour);
    DEBUG_PRINT(":");
    if (currentMinute < 10) DEBUG_PRINT("0");
    DEBUG_PRINT(currentMinute);
    DEBUG_PRINT("' -> ");
    //delay(1000);
    {
      // ===== Display current time =====
      displayTime(currentHour, currentMinute);
    }
    //else
    //{
    //  DEBUG_PRINT("ERROR: "); DEBUG_PRINT(resultval); DEBUG_PRINTLN(" Pattern validation failed!");
    //  delay(1000);
    //}
    delay(50);
  }
}

void checkPattern()
{
  testWords();

    FastLED.clear();
    uint8_t lengthPattern = strlen(testPattern);
    for(uint8_t n = 0; n < lengthPattern && testPattern[n] != '\0' ;n++)
    {
      char pattern[12];
      uint8_t patternPos = 0;
      while(testPattern[n] != '-' &&
           testPattern[n]  != '\0'   )
      {
        pattern[patternPos++] = testPattern[n++];
      }
      pattern[patternPos]='\0';
      int firstPos  = findWord(pattern, 0);
      int secondPos = findWord(pattern, 1);
      if(firstPos >= 0)
      {
        setWord(pattern, normalColor,0);
      }
      if(secondPos >= 0)
      {
        setWord(pattern, normalColor,1);
      }
      showLEDs();
      //delay(500);
      yield();
      FastLED.clear();
    }
}
