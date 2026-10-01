#include <common.inc>
#include <Updater.h>
#include <ca_cert.inc>
#include <sys/time.h>
#include <coredecls.h>   // settimeofday_cb: Benachrichtigung bei neuer SNTP-Zeit

RTC_DS1307 rtc;

// ════════════════════════════════════════════════════════════════
// NETZWERK
// ════════════════════════════════════════════════════════════════
ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;



// ════════════════════════════════════════════════════════════════
// CHARSOAP / ZEICHENSATZ
// ════════════════════════════════════════════════════════════════
void initCharsoap(char *_charsoap) {
    // Konvertiert UTF-8 Umlaute in charsoap zu ASCII in-place
    // charsoap muss bereits gefüllt sein (von strcpy_P)

    char temp[CHARSOAP_LEN * 2 + 1];  // Temporärer Buffer für Konvertierung
    int writePos = 0;
    int readPos = 0;
    int sourceLen = strlen(_charsoap);

    while (readPos < sourceLen && writePos < CHARSOAP_LEN) {
        unsigned char c = _charsoap[readPos];

        // UTF-8 Umlaute erkennen und zu lowercase konvertieren
        if (c == 0xC3 && readPos + 1 < sourceLen) {
            unsigned char next = _charsoap[readPos + 1];
            switch(next) {
                case 0x84: temp[writePos++] = 'a'; break;  // Ä → a
                case 0x96: temp[writePos++] = 'o'; break;  // Ö → o
                case 0x9C: temp[writePos++] = 'u'; break;  // Ü → u
                default:
                    temp[writePos++] = c;
                    if (writePos < CHARSOAP_LEN) temp[writePos++] = next;
                    break;
            }
            readPos += 2;
        } else {
            temp[writePos++] = c;
            readPos++;
        }
    }

    temp[writePos] = '\0';
    strcpy(_charsoap, temp);  // Konvertiertes Ergebnis zurück nach charsoap

    DEBUG_PRINTLN("✓ charsoap initialisiert (Umlaute → lowercase)");
    DEBUG_PRINTLN(_charsoap);
}

void loadCharsoap() {
    customCharsoap = EEPROM.read(ADDR_CHARSOAP_SET) == 1;

    if (customCharsoap) {
        DEBUG_PRINTLN("Lade charsoap aus EEPROM...");

        char rawData[256];
        int rawLen = 0;

        for (int i = 0; i < 200 && rawLen < 255; i++) {
            byte b = EEPROM.read(ADDR_CHARSOAP + i);
            if (b == 0 || b == 0xFF) break;
            rawData[rawLen++] = b;
        }
        rawData[rawLen] = '\0';

        DEBUG_PRINTF("Raw Length: %d bytes\n", rawLen);

        int writePos = 0;
        int readPos = 0;

        while (readPos < rawLen && writePos < CHARSOAP_LEN) {
            unsigned char c = rawData[readPos];

            if (c == 0xC3 && readPos + 1 < rawLen) {
                unsigned char next = rawData[readPos + 1];
                switch(next) {
                    case 0x84: charsoap[writePos++] = 'a'; break;
                    case 0x96: charsoap[writePos++] = 'o'; break;
                    case 0x9C: charsoap[writePos++] = 'u'; break;
                    case 0xA4: charsoap[writePos++] = 'a'; break;
                    case 0xB6: charsoap[writePos++] = 'o'; break;
                    case 0xBC: charsoap[writePos++] = 'u'; break;
                    default:
                        DEBUG_PRINTF("⚠ Unbekannt: 0xC3 0x%02X\n", next);
                        charsoap[writePos++] = c;
                        if (writePos < CHARSOAP_LEN)
                          charsoap[writePos++] = next;
                        break;
                }
                readPos += 2;
            } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
                charsoap[writePos++] = c;
                readPos++;
            } else if (c < 0x80) {
                readPos++;
            } else {
                if ((c & 0xE0) == 0xC0) readPos += 2;
                else if ((c & 0xF0) == 0xE0) readPos += 3;
                else if ((c & 0xF8) == 0xF0) readPos += 4;
                else readPos++;
            }
        }

        charsoap[writePos] = '\0';

        DEBUG_PRINTF("Konvertierte Länge: %d Zeichen\n", writePos);

        if (writePos == CHARSOAP_LEN) {
            DEBUG_PRINTLN("✓ Charsoap geladen (UTF-8 bereinigt)");
            DEBUG_PRINTLN(charsoap);

            if (rawLen != CHARSOAP_LEN) {
                DEBUG_PRINTLN("→ Speichere bereinigte Version...");
                for (int i = 0; i < CHARSOAP_LEN; i++) {
                    EEPROM.write(ADDR_CHARSOAP + i, charsoap[i]);
                }
                EEPROM.commit();
            }
        } else {
            DEBUG_PRINTF("❌ Falsche Länge: %d\n", writePos);
            strcpy_P(charsoap, (const char*)DEFAULT_CHARSOAP);
            initCharsoap(charsoap);  // UTF-8 zu ASCII konvertieren!
            customCharsoap = false;
            DEBUG_PRINTLN("→ Verwende Default");
        }
    } else {
        strcpy_P(charsoap, (const char*)DEFAULT_CHARSOAP);
        initCharsoap(charsoap);  // UTF-8 zu ASCII konvertieren!
        DEBUG_PRINTLN("✓ Standard charsoap");
    }
}

void saveCharsoap(const char* newCharsoap)
{
    char cleaned[CHARSOAP_LEN + 1];
    uint16_t writePos = 0;
    uint16_t readPos = 0;

    while (readPos < (uint16_t)strlen(newCharsoap) && writePos < CHARSOAP_LEN) {
        unsigned char c = newCharsoap[readPos];

        if (c == 0xC3 && readPos + 1 < (uint16_t)strlen(newCharsoap)) {
            unsigned char next = newCharsoap[readPos + 1];
            switch(next) {
                case 0x84: cleaned[writePos++] = 'a'; break;
                case 0x96: cleaned[writePos++] = 'o'; break;
                case 0x9C: cleaned[writePos++] = 'u'; break;
                case 0xA4: cleaned[writePos++] = 'a'; break;
                case 0xB6: cleaned[writePos++] = 'o'; break;
                case 0xBC: cleaned[writePos++] = 'u'; break;
                default:
                    DEBUG_PRINTF("❌ UTF-8: 0x%02X\n", next);
                    return;
            }
            readPos += 2;
        } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
            cleaned[writePos++] = c;
            readPos++;
        } else {
            DEBUG_PRINTF("❌ Zeichen: 0x%02X\n", c);
            return;
        }
    }

    cleaned[writePos] = '\0';

    if (writePos != CHARSOAP_LEN) {
        DEBUG_PRINTF("❌ saveCharsoap: ungueltige Laenge %u (erwartet %u)\n",
                     writePos, (unsigned)CHARSOAP_LEN);
        return;
    }

    for (uint16_t i = 0; i < CHARSOAP_LEN; i++) {
        EEPROM.write(ADDR_CHARSOAP + i, cleaned[i]);
    }
    EEPROM.write(ADDR_CHARSOAP_SET, 1);
    EEPROM.commit();

    strcpy(charsoap, cleaned);
    customCharsoap = true;

    DEBUG_PRINTLN("✓ Charsoap gespeichert");
}

void resetCharsoap() {
    strcpy_P(charsoap, (const char*)DEFAULT_CHARSOAP);
    initCharsoap(charsoap);  // UTF-8 zu ASCII konvertieren!
    customCharsoap = false;
    EEPROM.write(ADDR_CHARSOAP_SET, 0);
    EEPROM.commit();
}

// ════════════════════════════════════════════════════════════════
// SPECIAL WORD FUNKTIONEN
// ════════════════════════════════════════════════════════════════
void loadSpecialWords() {
    // Prüfen ob Custom Special Words gesetzt sind
    if (EEPROM.read(ADDR_SPECIAL_WORD_SET) == SPECIAL_WORD_MAGIC) {
        DEBUG_PRINTLN("Lade SPECIAL_WORD aus EEPROM...");

        // 3 Special Words laden
        for (int w = 0; w < MAXWORDS; w++) {
            int baseAddr = ADDR_SPECIAL_WORD_1 + (w * 12);
            for (int i = 0; i < 12; i++) {
                char c = EEPROM.read(baseAddr + i);
                SPECIAL_WORD[w][i] = c;
                if (c == '\0') break;
            }
            SPECIAL_WORD[w][11] = '\0';  // Sicherstellen, dass terminiert
        }

        DEBUG_PRINTLN("✓ SPECIAL_WORD aus EEPROM geladen");
        for (int w = 0; w < MAXWORDS; w++) {
            if (strlen(SPECIAL_WORD[w]) > 0) {
                DEBUG_PRINTF("  [%d]: %s\n", w, SPECIAL_WORD[w]);
            }
        }
    } else {
        // Erste Initialisierung: Default-Werte setzen
        DEBUG_PRINTLN("Setze Standard SPECIAL_WORD...");
        const char DEFAULT_SPECIAL_WORD[MAXWORDS][12] = {
            "DN9DAC",
            "",
            ""
        };

        for (int w = 0; w < MAXWORDS; w++) {
            strcpy((char*)SPECIAL_WORD[w], DEFAULT_SPECIAL_WORD[w]);
        }

        DEBUG_PRINTLN("✓ Standard SPECIAL_WORD gesetzt");
        for (int w = 0; w < MAXWORDS; w++) {
            if (strlen(SPECIAL_WORD[w]) > 0) {
                DEBUG_PRINTF("  [%d]: %s\n", w, SPECIAL_WORD[w]);
            }
        }
    }
}

void saveSpecialWords(const char words[MAXWORDS][12]) {
    DEBUG_PRINTLN("Speichere SPECIAL_WORD ins EEPROM...");

    // Validierung: max 11 Zeichen pro Wort
    for (int w = 0; w < MAXWORDS; w++) {
        if (strlen(words[w]) > 11) {
            DEBUG_PRINTF("❌ Wort %d zu lang: %d Zeichen\n", w, strlen(words[w]));
            return;
        }
    }

    // Speichern
    for (int w = 0; w < MAXWORDS; w++) {
        int baseAddr = ADDR_SPECIAL_WORD_1 + (w * 12);
        for (int i = 0; i < 12; i++) {
            char c = (i < (int) strlen(words[w])) ? words[w][i] : '\0';
            EEPROM.write(baseAddr + i, c);
        }

        // Ins globale Array kopieren
        strcpy((char*)SPECIAL_WORD[w], words[w]);
    }

    EEPROM.write(ADDR_SPECIAL_WORD_SET, SPECIAL_WORD_MAGIC);
    EEPROM.commit();

    DEBUG_PRINTLN("✓ SPECIAL_WORD gespeichert");
}

void resetSpecialWords() {
    DEBUG_PRINTLN("Setze SPECIAL_WORD auf Standard zurück...");

    // Standard-Werte wie in loadSpecialWords()/rgbPanel.cpp – frueher stand
    // hier "RWD", nach dem naechsten Neustart erschien dann "DN9DAC".
    const char DEFAULT_SPECIAL_WORD[MAXWORDS][12] = {
        "DN9DAC",
        "",
        ""
    };

    // Ins globale Array kopieren
    for (int w = 0; w < MAXWORDS; w++) {
        strcpy((char*)SPECIAL_WORD[w], DEFAULT_SPECIAL_WORD[w]);
    }

    EEPROM.write(ADDR_SPECIAL_WORD_SET, 0);
    EEPROM.commit();

    DEBUG_PRINTLN("✓ SPECIAL_WORD zurückgesetzt");
}

// ════════════════════════════════════════════════════════════════
// MINUTE LEDS FUNKTIONEN
// ════════════════════════════════════════════════════════════════
void loadMinuteLeds() {
    // Prüfen ob Custom Minute LEDs gesetzt sind
    if (EEPROM.read(ADDR_MINUTE_LEDS_SET) == MINUTE_LEDS_MAGIC) {
        DEBUG_PRINTLN("Lade MINUTE_LEDS aus EEPROM...");

        // 4 LED-Positionen laden
        for (int i = 0; i < 4; i++) {
            MINUTE_LEDS[i] = EEPROM.read(ADDR_MINUTE_LEDS + i);
        }

        DEBUG_PRINTLN("✓ MINUTE_LEDS aus EEPROM geladen");
        DEBUG_PRINTF("  Positionen: [%d, %d, %d, %d]\n",
                     MINUTE_LEDS[0], MINUTE_LEDS[1],
                     MINUTE_LEDS[2], MINUTE_LEDS[3]);
    } else {
        DEBUG_PRINTLN("✓ Standard MINUTE_LEDS (aus Code)");
    }
}

void saveMinuteLeds(const uint8_t leds[4]) {
    DEBUG_PRINTLN("Speichere MINUTE_LEDS ins EEPROM...");

    // Validierung: LEDs müssen im gültigen Bereich sein (0-120)
    for (int i = 0; i < 4; i++) {
        if (leds[i] >= NUM_LEDS) {
            DEBUG_PRINTF("❌ LED %d ungültig: %d (max %d)\n", i, leds[i], NUM_LEDS - 1);
            return;
        }
    }

    // Speichern
    for (int i = 0; i < 4; i++) {
        EEPROM.write(ADDR_MINUTE_LEDS + i, leds[i]);
        MINUTE_LEDS[i] = leds[i];
    }

    EEPROM.write(ADDR_MINUTE_LEDS_SET, MINUTE_LEDS_MAGIC);
    EEPROM.commit();

    DEBUG_PRINTLN("✓ MINUTE_LEDS gespeichert");
    DEBUG_PRINTF("  Positionen: [%d, %d, %d, %d]\n",
                 leds[0], leds[1], leds[2], leds[3]);
}

void resetMinuteLeds() {
    DEBUG_PRINTLN("Setze MINUTE_LEDS auf Standard zurück...");

    // Standard-Werte aus rgbPanel.cpp
    const uint8_t DEFAULT_MINUTE_LEDS[4] = {112, 114, 116, 118};

    // Ins globale Array kopieren
    for (int i = 0; i < 4; i++) {
        MINUTE_LEDS[i] = DEFAULT_MINUTE_LEDS[i];
    }

    EEPROM.write(ADDR_MINUTE_LEDS_SET, 0);
    EEPROM.commit();

    DEBUG_PRINTLN("✓ MINUTE_LEDS zurückgesetzt");
}

// ════════════════════════════════════════════════════════════════
// SPECIAL WORD INTERVAL FUNKTIONEN
// ════════════════════════════════════════════════════════════════
void loadSpecialWordInterval() {
    uint8_t stored = EEPROM.read(ADDR_SPECIAL_WORD_INTERVAL);

    // Validierung: nur erlaubte Werte
    if (stored == 1 || stored == 5 || stored == 10 ||
        stored == 15 || stored == 30 || stored == 60) {
        specialWordInterval = stored;
        DEBUG_PRINTF("✓ Spezialwort-Intervall aus EEPROM geladen: %d Minuten\n", specialWordInterval);
    } else {
        specialWordInterval = 60;  // Default: jede Stunde
        DEBUG_PRINTLN("✓ Standard Spezialwort-Intervall: 60 Minuten");
    }
}

void saveSpecialWordInterval(uint8_t interval) {
    // Validierung
    if (interval != 1 && interval != 5 && interval != 10 &&
        interval != 15 && interval != 30 && interval != 60) {
        DEBUG_PRINTF("❌ Ungültiges Intervall: %d\n", interval);
        return;
    }

    specialWordInterval = interval;
    EEPROM.write(ADDR_SPECIAL_WORD_INTERVAL, interval);
    EEPROM.commit();

    DEBUG_PRINTF("✓ Spezialwort-Intervall gespeichert: %d Minuten\n", interval);
}

void resetSpecialWordInterval() {
    specialWordInterval = 60;
    EEPROM.write(ADDR_SPECIAL_WORD_INTERVAL, 60);
    EEPROM.commit();
    DEBUG_PRINTLN("✓ Spezialwort-Intervall zurückgesetzt auf 60 Minuten");
}

void loadSpecialWordMode() {
    uint8_t stored = EEPROM.read(ADDR_SPECIAL_WORD_MODE);
    if (stored <= SPECIAL_WORD_MODE_PARALLEL) {
        specialWordMode = stored;
        DEBUG_PRINTF("✓ Spezialwort-Modus aus EEPROM geladen: %d\n", specialWordMode);
    } else {
        specialWordMode = SPECIAL_WORD_MODE_INTERVAL;
        DEBUG_PRINTLN("✓ Standard Spezialwort-Modus: Intervall");
    }
}

void saveSpecialWordMode(uint8_t mode) {
    if (mode > SPECIAL_WORD_MODE_PARALLEL) {
        DEBUG_PRINTF("❌ Ungültiger Spezialwort-Modus: %d\n", mode);
        return;
    }
    specialWordMode = mode;
    EEPROM.write(ADDR_SPECIAL_WORD_MODE, mode);
    EEPROM.commit();
    DEBUG_PRINTF("✓ Spezialwort-Modus gespeichert: %d\n", mode);
}

// Hilfsfunktion: Prüft, ob Spezialwort angezeigt werden soll
bool shouldShowSpecialWord(int minutes) {
    // Prüfe ob aktuelle Minute ein Vielfaches des Intervalls ist
    return (minutes % specialWordInterval) == 0;
}

// ════════════════════════════════════════════════════════════════
// ZEITBASIS
// ════════════════════════════════════════════════════════════════
// Die angezeigte Zeit ist bootTime + uptimeSeconds() (UTC) abzueglich der
// ESP-Driftkorrektur. bootTime wird bei jeder externen Zeitquelle (NTP,
// manuelle Eingabe, RTC) ueber anchorTime() neu gesetzt.

// Laufzeit seit Boot in Sekunden, ueberlauffest (millis() springt nach
// 49,7 Tagen auf 0). ALLE Rechnungen mit bootTime muessen diese Funktion
// statt millis()/1000 verwenden, sonst springt die Uhr nach dem Ueberlauf
// um ca. 17 h. loop() ruft sie laufend auf, der Ueberlauf wird also sicher
// erkannt.
unsigned long uptimeSeconds() {
    static unsigned long lastMillis = 0;
    static unsigned long overflows = 0;
    unsigned long m = millis();
    if (m < lastMillis) overflows++;
    lastMillis = m;
    return overflows * 4294967UL + m / 1000;
}

// Anker der laufenden Session: wann und aus welcher Quelle bootTime zuletzt
// gesetzt wurde. Die ESP-Driftkorrektur gilt nur ab diesem Anker – frueher
// wurde sie ab dem im EEPROM gespeicherten lastSyncTime einer frueheren
// Session gerechnet und damit nach jedem Neustart bzw. RTC-Abgleich
// zusaetzlich (und teils mit falschem Vorzeichen) angewendet.
static bool          anchorValid   = false;
static bool          anchorPrecise = false;  // NTP oder manuelle Eingabe (nicht RTC)
static unsigned long anchorUptime  = 0;      // uptimeSeconds() beim Anker
static unsigned long anchorMillis  = 0;      // millis() beim Anker (fuer ms-genaue Driftmessung)
static int64_t       anchorRealMs  = 0;      // Echtzeit (UTC, ms) beim Anker

void anchorTime(unsigned long realSec, bool precise, int64_t realMs = 0) {
    anchorUptime  = uptimeSeconds();
    anchorMillis  = millis();
    anchorRealMs  = realMs ? realMs : (int64_t)realSec * 1000;
    anchorValid   = true;
    anchorPrecise = precise;
    bootTime      = realSec - anchorUptime;
}

// Misst beim Eintreffen einer praezisen Zeit (NTP, manuell), wie weit die
// ESP-eigene Uhr seit dem letzten praezisen Anker abgewichen ist, und
// mittelt das in driftRate (s/Tag, + = ESP-Uhr geht vor). Muss VOR
// anchorTime() aufgerufen werden.
void measureEspDrift(int64_t realMsNow, unsigned long minSpanSec) {
    if (!anchorValid || !anchorPrecise) return;
    int64_t realSpanMs = realMsNow - anchorRealMs;
    if (realSpanMs < (int64_t)minSpanSec * 1000 || realSpanMs > 40LL * 86400000LL) return;
    unsigned long espSpanMs = millis() - anchorMillis;      // wrap-sicher (< 49 Tage)
    float driftSec = (float)((int64_t)espSpanMs - realSpanMs) / 1000.0f;
    float sample = driftSec * 86400000.0f / (float)realSpanMs;
    if (sample < -10.0f || sample > 10.0f) return;           // unplausibel (Quarz: wenige s/Tag)
    int n = syncCount < 50 ? syncCount : 50;                // gleitender Mittelwert
    driftRate = (driftRate * n + sample) / (n + 1);
    if (syncCount < 1000) syncCount++;
    DEBUG_PRINTF("  ESP-Drift: %.2f s/Tag (Messung %.2f)\n", driftRate, sample);
}

// ESP-Driftkorrektur seit dem Anker in Sekunden (wird von der Rohzeit abgezogen)
static long espDriftCorrection() {
    if (!anchorValid || driftRate == 0.0f) return 0;
    float days = (float)(uptimeSeconds() - anchorUptime) / 86400.0f;
    return lroundf(driftRate * days);
}

// Kleine Helfer fuer 32-Bit-Werte im EEPROM (Big Endian wie im Rest des Codes)
static uint32_t eepromReadU32(int addr) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v = (v << 8) | EEPROM.read(addr + i);
    return v;
}
static void eepromWriteU32(int addr, uint32_t v) {
    for (int i = 0; i < 4; i++) EEPROM.write(addr + i, (v >> (24 - 8 * i)) & 0xFF);
}
static float eepromReadFloat(int addr) {
    uint32_t v = eepromReadU32(addr);
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}
static void eepromWriteFloat(int addr, float f) {
    uint32_t v;
    memcpy(&v, &f, sizeof(v));
    eepromWriteU32(addr, v);
}

// Schreibt die aktuelle Uhrzeit nach ADDR_TIMESTAMP ("letzte bekannte Zeit"
// fuer die Stromausfall-Erkennung). Wird stuendlich aus loop() und bei jeder
// NTP-Zeit gerufen.
unsigned long lastTimePersistMs = 0;
void saveLastKnownTime(bool commit = true) {
    eepromWriteU32(ADDR_TIMESTAMP, bootTime + uptimeSeconds() - espDriftCorrection());
    lastTimePersistMs = millis();
    if (commit) EEPROM.commit();
}

void loadDriftRate() {
    lastSyncTime = eepromReadU32(ADDR_LAST_SYNC);
    driftRate = eepromReadFloat(ADDR_DRIFT_RATE);

    syncCount = 0;
    syncCount |= EEPROM.read(ADDR_SYNC_COUNT) << 8;
    syncCount |= EEPROM.read(ADDR_SYNC_COUNT + 1);

    // Frisches EEPROM liefert 0xFF... – muss ebenfalls verworfen werden
    if (lastSyncTime < 1735689600UL || lastSyncTime > 4102444800UL) lastSyncTime = 0;
    if (isnan(driftRate) || driftRate < -10.0 || driftRate > 10.0) driftRate = 0.0;
    if (syncCount < 0 || syncCount > 1000) syncCount = 0;
}

void saveDriftRate(bool commit = true) {
    eepromWriteU32(ADDR_LAST_SYNC, lastSyncTime);
    eepromWriteFloat(ADDR_DRIFT_RATE, driftRate);
    EEPROM.write(ADDR_SYNC_COUNT, (syncCount >> 8) & 0xFF);
    EEPROM.write(ADDR_SYNC_COUNT + 1, syncCount & 0xFF);
    if (commit) EEPROM.commit();
}

// ════════════════════════════════════════════════════════════════
// RTC-DRIFT-KALIBRIERUNG
// ════════════════════════════════════════════════════════════════
// Bei jeder neuen NTP-Zeit wird der Versatz "RTC minus Echtzeit" auf wenige
// Millisekunden genau gemessen (Warten auf den Sekundenwechsel der RTC). Die
// Aenderung des Versatzes seit dem letzten praezisen Referenzpunkt ist die
// Drift dieses Intervalls; alle Intervalle werden aufsummiert
// (Summe Drift / Summe Zeit = Rate). Weicht die RTC um mehr als 0,5 s ab,
// wird sie exakt auf den Sekundenwechsel nachgestellt, damit sie offline
// moeglichst genau startet. Offline rechnet getCorrectedRTCTime() den
// Versatz am Referenzpunkt plus Rate x Zeit seitdem heraus.
// Bei fixierter Rate wird nur noch nachgestellt, nicht mehr gemessen.

void loadRTCDrift() {
    if (EEPROM.read(ADDR_RTCCAL_MAGIC) != RTCCAL_MAGIC) {
        // Frisches EEPROM oder altes Layout: sauber mit Defaults starten
        rtcDriftRate = 0.0f;
        rtcDriftLocked = false;
        rtcRefTime = 0;
        rtcRefOffset = 0.0f;
        rtcRefPrecise = false;
        rtcCalDriftSum = 0.0f;
        rtcCalTimeSum = 0;
        DEBUG_PRINTLN("RTC-Drift: keine Kalibrierung gespeichert");
        return;
    }
    rtcDriftRate   = eepromReadFloat(ADDR_RTCCAL_RATE);
    rtcDriftLocked = (EEPROM.read(ADDR_RTCCAL_LOCKED) == RTC_DRIFT_LOCK_MAGIC);
    rtcRefTime     = eepromReadU32(ADDR_RTCCAL_REF_TIME);
    rtcRefOffset   = eepromReadFloat(ADDR_RTCCAL_REF_OFFSET);
    rtcRefPrecise  = (EEPROM.read(ADDR_RTCCAL_REF_PRECISE) == 1);
    rtcCalDriftSum = eepromReadFloat(ADDR_RTCCAL_DRIFT_SUM);
    rtcCalTimeSum  = eepromReadU32(ADDR_RTCCAL_TIME_SUM);

    if (isnan(rtcDriftRate) || fabsf(rtcDriftRate) > 60.0f) rtcDriftRate = 0.0f;
    if (isnan(rtcRefOffset) || fabsf(rtcRefOffset) > 100000.0f ||
        rtcRefTime < 1735689600UL || rtcRefTime > 4102444800UL) {
        rtcRefTime = 0;
        rtcRefOffset = 0.0f;
        rtcRefPrecise = false;
    }
    if (isnan(rtcCalDriftSum) || rtcCalTimeSum > 3153600000UL) {
        rtcCalDriftSum = 0.0f;
        rtcCalTimeSum = 0;
    }
    DEBUG_PRINTF("RTC-Drift: %.3f s/Tag, Messdauer %lu h%s\n",
                 rtcDriftRate, rtcCalTimeSum / 3600, rtcDriftLocked ? " (fixiert)" : "");
}

void saveRTCDrift(bool commit = true) {
    EEPROM.write(ADDR_RTCCAL_MAGIC, RTCCAL_MAGIC);
    eepromWriteFloat(ADDR_RTCCAL_RATE, rtcDriftRate);
    EEPROM.write(ADDR_RTCCAL_LOCKED, rtcDriftLocked ? RTC_DRIFT_LOCK_MAGIC : 0);
    eepromWriteU32(ADDR_RTCCAL_REF_TIME, rtcRefTime);
    eepromWriteFloat(ADDR_RTCCAL_REF_OFFSET, rtcRefOffset);
    EEPROM.write(ADDR_RTCCAL_REF_PRECISE, rtcRefPrecise ? 1 : 0);
    eepromWriteFloat(ADDR_RTCCAL_DRIFT_SUM, rtcCalDriftSum);
    eepromWriteU32(ADDR_RTCCAL_TIME_SUM, rtcCalTimeSum);
    if (commit) EEPROM.commit();
}

// Liest die RTC und rechnet Versatz + Drift seit dem Referenzpunkt heraus.
// Liefert 0, wenn keine RTC vorhanden ist oder sie steht.
unsigned long getCorrectedRTCTime() {
    if (!rtcPresent || !rtc.isrunning()) return 0;
    unsigned long raw = rtc.now().unixtime();
    if (rtcRefTime == 0) return raw;
    float days = (raw > rtcRefTime) ? (float)(raw - rtcRefTime) / 86400.0f : 0.0f;
    long corr = lroundf(rtcRefOffset + rtcDriftRate * days);
    return (unsigned long)((int64_t)raw - corr);
}

// Misst "RTC minus Echtzeit" in Sekunden auf wenige ms genau: wartet auf den
// naechsten Sekundenwechsel der RTC und vergleicht ihn mit der NTP-gestuetzten
// Systemzeit. Die RTC liefert nur ganze Sekunden – ohne diesen Trick waere
// jede Messung um bis zu +-1 s verrauscht (bei 1 h Messdauer +-24 s/Tag).
// Blockiert max. ~1,2 s.
bool measureRtcOffset(float &offsetSec) {
    if (!rtcPresent || !rtc.isrunning()) return false;
    uint32_t sec0 = rtc.now().unixtime();
    uint32_t t0 = millis();
    while (millis() - t0 < 1200) {
        uint32_t sec1 = rtc.now().unixtime();
        if (sec1 != sec0) {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            double real = (double)tv.tv_sec + tv.tv_usec / 1e6;
            offsetSec = (float)((double)sec1 - real);
            return true;
        }
        yield();
    }
    return false;
}

// Stellt die RTC exakt auf die Systemzeit: wartet auf den naechsten vollen
// Sekundenwechsel und schreibt dann (der DS1307 setzt beim Schreiben des
// Sekundenregisters seinen Teiler zurueck). Blockiert max. ~1 s.
void setRtcPrecise() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t target = tv.tv_sec + 1;
    uint32_t t0 = millis();
    while (tv.tv_sec < target && millis() - t0 < 1500) {
        yield();
        gettimeofday(&tv, nullptr);
    }
    rtc.adjust(DateTime((uint32_t)tv.tv_sec));
}

// Bei jeder neuen NTP-Zeit aufgerufen (nicht bei manueller Eingabe – die ist
// fuer eine Driftmessung zu ungenau). Speichert nicht selbst – der Aufrufer
// schreibt alle Werte gemeinsam mit einem einzigen EEPROM.commit().
void updateRTCDriftCalibration() {
    if (!rtcPresent) return;

    struct timeval tv;
    float off;
    if (!measureRtcOffset(off)) {
        // RTC steht (z.B. Batterie leer) -> neu stellen, Versatz noch unbekannt
        setRtcPrecise();
        gettimeofday(&tv, nullptr);
        rtcRefTime = tv.tv_sec;
        rtcRefOffset = 0.0f;
        rtcRefPrecise = false;
        DEBUG_PRINTLN("RTC-Drift: RTC lief nicht - neu gestellt");
        return;
    }
    gettimeofday(&tv, nullptr);
    unsigned long now = tv.tv_sec;

    bool refUsable = rtcRefPrecise && rtcRefTime > 0 && now > rtcRefTime;
    if (refUsable && now - rtcRefTime < 1800) return;  // Intervall noch zu kurz

    if (refUsable && !rtcDriftLocked) {
        unsigned long span = now - rtcRefTime;
        float drift = off - rtcRefOffset;
        float sample = drift * 86400.0f / (float)span;
        if (fabsf(sample) <= 60.0f) {
            rtcCalDriftSum += drift;
            rtcCalTimeSum  += span;
            rtcDriftRate = rtcCalDriftSum * 86400.0f / (float)rtcCalTimeSum;
            DEBUG_PRINTF("RTC-Drift: %+.3f s in %.1f h -> %.3f s/Tag (Messdauer %lu h)\n",
                         drift, span / 3600.0f, rtcDriftRate, rtcCalTimeSum / 3600);
        } else {
            DEBUG_PRINTF("RTC-Drift: Messung verworfen (%.1f s/Tag unplausibel)\n", sample);
        }
    }

    // Neuer Referenzpunkt. Bei mehr als 0,5 s Abweichung die RTC exakt
    // nachstellen und den Restversatz gleich nochmal messen.
    if (fabsf(off) > 0.5f) {
        setRtcPrecise();
        if (!measureRtcOffset(off)) off = 0.0f;
        gettimeofday(&tv, nullptr);
        now = tv.tv_sec;
    }
    rtcRefTime = now;
    rtcRefOffset = off;
    rtcRefPrecise = true;
}

// Manuelle Zeiteingabe: RTC stellen. Die Referenz gilt als ungenau
// (Browserzeit, Netzlaufzeit) und wird fuer die Driftmessung nicht genutzt –
// die naechste NTP-Zeit setzt wieder eine praezise Referenz.
void rtcSetManually(unsigned long utcSec) {
    if (!rtcPresent) return;
    rtc.adjust(DateTime((uint32_t)utcSec));
    rtcRefTime = utcSec;
    rtcRefOffset = 0.0f;
    rtcRefPrecise = false;
    saveRTCDrift();
}

// ════════════════════════════════════════════════════════════════
// EEPROM / PERSISTENZ
// ════════════════════════════════════════════════════════════════
void loadConfig() {
    isConfigured = EEPROM.read(ADDR_CONFIGURED);
    
    if (isConfigured == MAGIC_BYTE_INIT) {
        brightness = EEPROM.read(ADDR_BRIGHTNESS);
        if (brightness < 10 || brightness > 80) brightness = 80;

        normalColor.r = EEPROM.read(ADDR_COLOR_R);
        normalColor.g = EEPROM.read(ADDR_COLOR_G);
        normalColor.b = EEPROM.read(ADDR_COLOR_B);
        useRainbow = (EEPROM.read(ADDR_USE_RAINBOW) == RAINBOW_MAGIC);

        specialColor.r = EEPROM.read(ADDR_SPECIAL_R);
        specialColor.g = EEPROM.read(ADDR_SPECIAL_G);
        specialColor.b = EEPROM.read(ADDR_SPECIAL_B);
        {
            uint8_t sb = EEPROM.read(ADDR_SPECIAL_BRIGHTNESS);
            specialBrightness = (sb > 100 || sb == 0xFF) ? 100 : sb;
        }

        unsigned long savedTime = 0;
        savedTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP)) << 24;
        savedTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP + 1)) << 16;
        savedTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP + 2)) << 8;
        savedTime |= EEPROM.read(ADDR_TIMESTAMP + 3);

        if (savedTime > 1735689600 && savedTime < 2147483647) {
            bootTime = savedTime;
        }

        DEBUG_PRINTLN("✓ Konfiguration geladen");
        DEBUG_PRINTF("  Helligkeit: %d\n", brightness);
        DEBUG_PRINTF("  Farbe: RGB(%d,%d,%d)\n", normalColor.r, normalColor.g, normalColor.b);
    }
    else
    {
        // ════════════════════════════════════════════════════════════════
        // ERSTE INITIALISIERUNG - DEFAULT_CHARSOAP ins EEPROM schreiben
        // ════════════════════════════════════════════════════════════════
        DEBUG_PRINTLN("╔════════════════════════════════════════════════════╗");
        DEBUG_PRINTLN("║   ERSTE INITIALISIERUNG                            ║");
        DEBUG_PRINTLN("╚════════════════════════════════════════════════════╝");

        // DEFAULT_CHARSOAP vorbereiten (UTF-8 → ASCII)
        strcpy_P(charsoap, (const char*)DEFAULT_CHARSOAP);
        initCharsoap(charsoap);

        DEBUG_PRINTLN("→ Speichere DEFAULT_CHARSOAP ins EEPROM...");

        // Ins EEPROM schreiben
        for (int i = 0; i < CHARSOAP_LEN; i++) {
            EEPROM.write(ADDR_CHARSOAP + i, charsoap[i]);
        }
        EEPROM.write(ADDR_CHARSOAP_SET, 1);

        customCharsoap = true;

        DEBUG_PRINTLN("✓ DEFAULT_CHARSOAP ins EEPROM gespeichert");
        DEBUG_PRINTF("  Länge: %d Zeichen\n", strlen(charsoap));

        // Weitere Initialisierungen können hier folgen
        // SPECIAL_WORD und MINUTE_LEDS werden separat initialisiert

        EEPROM.commit();
    }
}

void saveConfig() {
    EEPROM.write(ADDR_BRIGHTNESS, brightness);
    EEPROM.write(ADDR_COLOR_R, normalColor.r);
    EEPROM.write(ADDR_COLOR_G, normalColor.g);
    EEPROM.write(ADDR_COLOR_B, normalColor.b);
    EEPROM.write(ADDR_USE_RAINBOW, useRainbow ? RAINBOW_MAGIC : 0);
    EEPROM.write(ADDR_SPECIAL_R, specialColor.r);
    EEPROM.write(ADDR_SPECIAL_G, specialColor.g);
    EEPROM.write(ADDR_SPECIAL_B, specialColor.b);
    EEPROM.write(ADDR_SPECIAL_BRIGHTNESS, specialBrightness);
    EEPROM.write(ADDR_CONFIGURED, MAGIC_BYTE_INIT);

    // Aktuelle Uhrzeit (nicht bootTime roh) als "letzte bekannte Zeit" fuer
    // detectPowerLossWithRTC() ablegen – committet wird gemeinsam unten.
    saveLastKnownTime(false);

    EEPROM.commit();
}


// ════════════════════════════════════════════════════════════════
// OTA VERSION MANAGEMENT
// ════════════════════════════════════════════════════════════════
void saveOTAVersion() {
    for (int i = 0; i < 32; i++) {
        EEPROM.write(ADDR_OTA_VERSION + i, firmwareVersion[i]);
        if (firmwareVersion[i] == '\0') break;
    }

    // Build-Datum hat 20 Bytes INKLUSIVE Nullterminator – frueher landete
    // die '\0' auf ADDR_OTA_FLAGS (218) und loeschte das OTA-Erfolgs-Flag.
    const char* buildDate = BUILD_DATE;
    const char* buildTime = BUILD_TIME;
    int idx = 0;
    for (uint8_t i = 0; i < strlen(buildDate) && idx < 11; i++, idx++) {
        EEPROM.write(ADDR_OTA_BUILD_DATE + idx, buildDate[i]);
    }
    EEPROM.write(ADDR_OTA_BUILD_DATE + idx++, ' ');
    for (uint8_t i = 0; i < strlen(buildTime) && idx < 19; i++, idx++) {
        EEPROM.write(ADDR_OTA_BUILD_DATE + idx, buildTime[i]);
    }
    EEPROM.write(ADDR_OTA_BUILD_DATE + idx, '\0');

    EEPROM.commit();
    DEBUG_PRINTLN("✓ OTA Version gespeichert: " + String(firmwareVersion));
}

void loadOTAVersion() {
    for (int i = 0; i < 32; i++) {
        uint8_t c = EEPROM.read(ADDR_OTA_VERSION + i);
        if (c == 0 || c == 0xFF) {
            firmwareVersion[i] = '\0';
            break;
        }
        firmwareVersion[i] = (char)c;
    }
    firmwareVersion[sizeof(firmwareVersion) - 1] = '\0';

    // Version aus dem Build ableiten und bei Abweichung (z.B. nach einem
    // OTA-Update) aktualisieren – frueher blieb die erste je installierte
    // Version fuer immer im EEPROM stehen.
    char current[sizeof(firmwareVersion)];
    generateVersion(current, sizeof(current));
    if (strcmp(firmwareVersion, current) != 0) {
        strncpy(firmwareVersion, current, sizeof(firmwareVersion) - 1);
        firmwareVersion[sizeof(firmwareVersion) - 1] = '\0';
        saveOTAVersion();
    }

    DEBUG_PRINTLN("Firmware Version: " + String(firmwareVersion));
}

// ════════════════════════════════════════════════════════════════
// WIFI STATION CONFIG MANAGEMENT
// ════════════════════════════════════════════════════════════════
void loadWiFiStationConfig() {
    // Station enabled flag
    staEnabled = (EEPROM.read(ADDR_STA_ENABLED) == STA_ENABLED_MAGIC);

    if (!staEnabled) {
        DEBUG_PRINTLN("WiFi Station nicht konfiguriert");
        return;
    }

    // SSID laden
    for (int i = 0; i < 32; i++) {
        byte c = EEPROM.read(ADDR_STA_SSID + i);
        if (c == 0 || c == 0xFF) {
            staSsid[i] = '\0';
            break;
        }
        staSsid[i] = c;
    }
    staSsid[31] = '\0';  // Sicherheit

    // Password laden
    for (int i = 0; i < 64; i++) {
        byte c = EEPROM.read(ADDR_STA_PASSWORD + i);
        if (c == 0 || c == 0xFF) {
            staPassword[i] = '\0';
            break;
        }
        staPassword[i] = c;
    }
    staPassword[63] = '\0';

    // DHCP flag
    staDhcp = (EEPROM.read(ADDR_STA_DHCP) == 0x01);

    // Statische IP-Konfiguration (wenn nicht DHCP)
    if (!staDhcp) {
        staIP[0] = EEPROM.read(ADDR_STA_IP);
        staIP[1] = EEPROM.read(ADDR_STA_IP + 1);
        staIP[2] = EEPROM.read(ADDR_STA_IP + 2);
        staIP[3] = EEPROM.read(ADDR_STA_IP + 3);

        staGateway[0] = EEPROM.read(ADDR_STA_GATEWAY);
        staGateway[1] = EEPROM.read(ADDR_STA_GATEWAY + 1);
        staGateway[2] = EEPROM.read(ADDR_STA_GATEWAY + 2);
        staGateway[3] = EEPROM.read(ADDR_STA_GATEWAY + 3);

        staSubnet[0] = EEPROM.read(ADDR_STA_SUBNET);
        staSubnet[1] = EEPROM.read(ADDR_STA_SUBNET + 1);
        staSubnet[2] = EEPROM.read(ADDR_STA_SUBNET + 2);
        staSubnet[3] = EEPROM.read(ADDR_STA_SUBNET + 3);

        staDNS[0] = EEPROM.read(ADDR_STA_DNS);
        staDNS[1] = EEPROM.read(ADDR_STA_DNS + 1);
        staDNS[2] = EEPROM.read(ADDR_STA_DNS + 2);
        staDNS[3] = EEPROM.read(ADDR_STA_DNS + 3);
    }

    // WPA2-Enterprise (PEAP/MS-CHAPv2)
    staEnterprise = (EEPROM.read(ADDR_STA_ENTERPRISE_ENABLED) == STA_ENTERPRISE_MAGIC);
    for (int i = 0; i < 64; i++) {
        byte c = EEPROM.read(ADDR_STA_ENTERPRISE_IDENTITY + i);
        if (c == 0 || c == 0xFF) {
            staIdentity[i] = '\0';
            break;
        }
        staIdentity[i] = c;
    }
    staIdentity[63] = '\0';

    // Outer/Anonymous Identity laden. Leer ist ein gueltiger, gewollter Wert
    // (= Benutzername wird als Outer Identity gesendet, wie bei Android).
    // Frueher wurde ein bewusst leer gespeichertes Feld beim Laden wieder zu
    // "anonymous" – das Feld liess sich also nie dauerhaft leeren.
    for (int i = 0; i < 64; i++) {
        byte c = EEPROM.read(ADDR_STA_ENTERPRISE_ANON_ID + i);
        if (c == 0 || c == 0xFF) {
            staAnonIdentity[i] = '\0';
            break;
        }
        staAnonIdentity[i] = c;
    }
    staAnonIdentity[63] = '\0';

    // DHCP-Hostname laden (Default leer)
    for (int i = 0; i < 64; i++) {
        byte c = EEPROM.read(ADDR_STA_HOSTNAME + i);
        if (c == 0 || c == 0xFF) {
            staHostname[i] = '\0';
            break;
        }
        staHostname[i] = c;
    }
    staHostname[63] = '\0';

    DEBUG_PRINTF("✓ WiFi Station SSID: %s\n", staSsid);
    DEBUG_PRINTF("  DHCP: %s\n", staDhcp ? "Ja" : "Nein");
    DEBUG_PRINTF("  Enterprise: %s\n", staEnterprise ? "Ja" : "Nein");
    if (!staDhcp) {
        DEBUG_PRINTF("  IP: %s\n", staIP.toString().c_str());
        DEBUG_PRINTF("  Gateway: %s\n", staGateway.toString().c_str());
    }
}

void saveWiFiStationConfig() {
    // Enabled flag
    EEPROM.write(ADDR_STA_ENABLED, staEnabled ? STA_ENABLED_MAGIC : 0);

    // SSID speichern
    for (int i = 0; i < 32; i++) {
        EEPROM.write(ADDR_STA_SSID + i, staSsid[i]);
        if (staSsid[i] == '\0') break;
    }

    // Password speichern
    for (int i = 0; i < 64; i++) {
        EEPROM.write(ADDR_STA_PASSWORD + i, staPassword[i]);
        if (staPassword[i] == '\0') break;
    }

    // DHCP flag
    EEPROM.write(ADDR_STA_DHCP, staDhcp ? 0x01 : 0x00);

    // Statische IP (immer speichern, auch wenn DHCP aktiv)
    EEPROM.write(ADDR_STA_IP, staIP[0]);
    EEPROM.write(ADDR_STA_IP + 1, staIP[1]);
    EEPROM.write(ADDR_STA_IP + 2, staIP[2]);
    EEPROM.write(ADDR_STA_IP + 3, staIP[3]);

    EEPROM.write(ADDR_STA_GATEWAY, staGateway[0]);
    EEPROM.write(ADDR_STA_GATEWAY + 1, staGateway[1]);
    EEPROM.write(ADDR_STA_GATEWAY + 2, staGateway[2]);
    EEPROM.write(ADDR_STA_GATEWAY + 3, staGateway[3]);

    EEPROM.write(ADDR_STA_SUBNET, staSubnet[0]);
    EEPROM.write(ADDR_STA_SUBNET + 1, staSubnet[1]);
    EEPROM.write(ADDR_STA_SUBNET + 2, staSubnet[2]);
    EEPROM.write(ADDR_STA_SUBNET + 3, staSubnet[3]);

    EEPROM.write(ADDR_STA_DNS, staDNS[0]);
    EEPROM.write(ADDR_STA_DNS + 1, staDNS[1]);
    EEPROM.write(ADDR_STA_DNS + 2, staDNS[2]);
    EEPROM.write(ADDR_STA_DNS + 3, staDNS[3]);

    // WPA2-Enterprise
    EEPROM.write(ADDR_STA_ENTERPRISE_ENABLED, staEnterprise ? STA_ENTERPRISE_MAGIC : 0);
    for (int i = 0; i < 64; i++) {
        EEPROM.write(ADDR_STA_ENTERPRISE_IDENTITY + i, staIdentity[i]);
        if (staIdentity[i] == '\0') break;
    }
    for (int i = 0; i < 64; i++) {
        EEPROM.write(ADDR_STA_ENTERPRISE_ANON_ID + i, staAnonIdentity[i]);
        if (staAnonIdentity[i] == '\0') break;
    }
    for (int i = 0; i < 64; i++) {
        EEPROM.write(ADDR_STA_HOSTNAME + i, staHostname[i]);
        if (staHostname[i] == '\0') break;
    }

    EEPROM.commit();
    DEBUG_PRINTLN("✓ WiFi Station Config gespeichert");
}

// ════════════════════════════════════════════════════════════════
// NTP CONFIG MANAGEMENT
// ════════════════════════════════════════════════════════════════
void loadNTPConfig() {
    ntpEnabled = (EEPROM.read(ADDR_NTP_ENABLED) == NTP_ENABLED_MAGIC);

    if (!ntpEnabled) {
        DEBUG_PRINTLN("NTP nicht aktiviert");
        return;
    }

    // NTP Server laden
    for (int i = 0; i < 32; i++) {
        byte c = EEPROM.read(ADDR_NTP_SERVER + i);
        if (c == 0 || c == 0xFF) {
            ntpServer[i] = '\0';
            break;
        }
        ntpServer[i] = c;
    }
    ntpServer[31] = '\0';

    // Letzter Sync-Timestamp
    lastNtpSync = 0;
    lastNtpSync |= ((unsigned long)EEPROM.read(ADDR_NTP_LAST_SYNC)) << 24;
    lastNtpSync |= ((unsigned long)EEPROM.read(ADDR_NTP_LAST_SYNC + 1)) << 16;
    lastNtpSync |= ((unsigned long)EEPROM.read(ADDR_NTP_LAST_SYNC + 2)) << 8;
    lastNtpSync |= EEPROM.read(ADDR_NTP_LAST_SYNC + 3);

    DEBUG_PRINTF("✓ NTP Server: %s\n", ntpServer);
    DEBUG_PRINTF("  Letzter Sync: %lu\n", lastNtpSync);
}

void saveNTPConfig(bool commit = true) {
    EEPROM.write(ADDR_NTP_ENABLED, ntpEnabled ? NTP_ENABLED_MAGIC : 0);

    // NTP Server speichern
    for (int i = 0; i < 32; i++) {
        EEPROM.write(ADDR_NTP_SERVER + i, ntpServer[i]);
        if (ntpServer[i] == '\0') break;
    }

    // Letzter Sync
    EEPROM.write(ADDR_NTP_LAST_SYNC, (lastNtpSync >> 24) & 0xFF);
    EEPROM.write(ADDR_NTP_LAST_SYNC + 1, (lastNtpSync >> 16) & 0xFF);
    EEPROM.write(ADDR_NTP_LAST_SYNC + 2, (lastNtpSync >> 8) & 0xFF);
    EEPROM.write(ADDR_NTP_LAST_SYNC + 3, lastNtpSync & 0xFF);

    if (commit) EEPROM.commit();
}

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS CONFIG MANAGEMENT
// ════════════════════════════════════════════════════════════════
void loadAutoBrightnessConfig() {
    autoBrightnessEnabled = (EEPROM.read(ADDR_AUTO_BRIGHTNESS_ENABLED) == AUTO_BRIGHTNESS_MAGIC);

    if (!autoBrightnessEnabled) {
        DEBUG_PRINTLN("Auto-Brightness nicht aktiviert");
        return;
    }

    // Min ADC-Wert laden (2 bytes)
    autoBrightnessMinADC = (EEPROM.read(ADDR_AUTO_BRIGHTNESS_MIN_ADC) << 8) |
                           EEPROM.read(ADDR_AUTO_BRIGHTNESS_MIN_ADC + 1);

    // Max ADC-Wert laden (2 bytes)
    autoBrightnessMaxADC = (EEPROM.read(ADDR_AUTO_BRIGHTNESS_MAX_ADC) << 8) |
                           EEPROM.read(ADDR_AUTO_BRIGHTNESS_MAX_ADC + 1);

    // Min/Max Helligkeit laden (1 byte each)
    autoBrightnessMin = EEPROM.read(ADDR_AUTO_BRIGHTNESS_MIN);
    autoBrightnessMax = EEPROM.read(ADDR_AUTO_BRIGHTNESS_MAX);

    // Validierung (frisches EEPROM liefert 0xFFFF)
    if (autoBrightnessMinADC > 1003) autoBrightnessMinADC = 200;
    if (autoBrightnessMaxADC > 1023 || autoBrightnessMaxADC <= autoBrightnessMinADC) {
        autoBrightnessMaxADC = (autoBrightnessMinADC < 820) ? 820 : autoBrightnessMinADC + 20;
    }
    if (autoBrightnessMin > 80) autoBrightnessMin = 0;
    if (autoBrightnessMax > 80 || autoBrightnessMax == 0) autoBrightnessMax = 80;
    if (autoBrightnessMin > autoBrightnessMax) autoBrightnessMin = 0;

    DEBUG_PRINTF("✓ Auto-Brightness: ADC=%d-%d, Brightness=%d-%d\n",
                 autoBrightnessMinADC, autoBrightnessMaxADC,
                 autoBrightnessMin, autoBrightnessMax);
}

void saveAutoBrightnessConfig() {
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_ENABLED, autoBrightnessEnabled ? AUTO_BRIGHTNESS_MAGIC : 0);

    // Min ADC-Wert speichern (2 bytes)
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MIN_ADC, (autoBrightnessMinADC >> 8) & 0xFF);
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MIN_ADC + 1, autoBrightnessMinADC & 0xFF);

    // Max ADC-Wert speichern (2 bytes)
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MAX_ADC, (autoBrightnessMaxADC >> 8) & 0xFF);
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MAX_ADC + 1, autoBrightnessMaxADC & 0xFF);

    // Min/Max Helligkeit speichern (1 byte each)
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MIN, autoBrightnessMin);
    EEPROM.write(ADDR_AUTO_BRIGHTNESS_MAX, autoBrightnessMax);

    EEPROM.commit();
    DEBUG_PRINTF("✓ Auto-Brightness: ADC=%d-%d, Brightness=%d-%d gespeichert\n",
                 autoBrightnessMinADC, autoBrightnessMaxADC,
                 autoBrightnessMin, autoBrightnessMax);
}

int setHour(int hour, CRGB color) {
    // Stundenwörter: Bei Rückwärtssuche immer occurrence=0 (letztes Vorkommen im String)
    // Dies ermöglicht "X VOR X" Anzeigen (z.B. FÜNF VOR FÜNF, ZEHN VOR ZEHN)
    switch(hour) {
        case 1:  return setWord("EINS"  , color, 0, true);
        case 2:  return setWord("ZWEI"  , color, 0, true);
        case 3:  return setWord("DREI"  , color, 0, true);
        case 4:  return setWord("VIER"  , color, 0, true);
        case 5:  return setWord("FuNF"  , color, 0, true);   // Nutzt "FuNF" in "ZWoLFuNF" für Stunde
        case 6:  return setWord("SECHS" , color, 0, true);
        case 7:  return setWord("SIEBEN", color, 0, true);
        case 8:  return setWord("ACHT"  , color, 0, true);
        case 9:  return setWord("NEUN"  , color, 0, true);
        case 10: return setWord("ZEHN"  , color, 0, true);
        case 11: return setWord("ELF"   , color, 0, true);
        case 12: return setWord("ZWoLF" , color, 0, true);
        case 13: return setWord("EIN"   , color, 0, true);
    }
    return -1;  // ungueltige Stunde (frueher: kein return -> undefiniertes Verhalten)
}

String getHourName(int hour)
{
    const char* hourNames[] = {
        "ZWÖLF", "EINS", "ZWEI", "DREI", "VIER", "FÜNF",
        "SECHS", "SIEBEN", "ACHT", "NEUN", "ZEHN", "ELF", "ZWÖLF", "EIN"
    };
    
    if (hour == 0) hour = 12;
    return String(hourNames[hour]);
}

void displayTime(int hours, int minutes)
{
  DEBUG_PRINT("displayTime Zeit: '");
  if (hours < 10) DEBUG_PRINT("0");
  DEBUG_PRINT(hours);
  DEBUG_PRINT(":");
  if (minutes < 10) DEBUG_PRINT("0");
  DEBUG_PRINT(minutes);
  DEBUG_PRINT("' -> ");
  
  //fadeOutAll(200, 15);
  // Bewusst KEIN FastLED.setBrightness() hier: die Helligkeit kommt entweder
  // aus dem manuellen User-Setting (in handleSave/setup gesetzt) oder aus
  // updateBrightness(). Wuerden wir hier hartcodieren, wuerde der Auto-
  // Wert jede Minute ueberschrieben werden.
  FastLED.clear();
  showLEDs();
  yield();
  
  CharGraphTimeWords result;
  int8_t resultval = getCharGraphWords(charsoap, testPattern, hours, minutes, result);
  
  if (resultval == 0)
  {
    uint8_t hasUhr = 0;
    char wordBuf[16];  // Buffer für PROGMEM-String
    
    // Letztes Wort aus PROGMEM lesen
    strcpy_P(wordBuf, (PGM_P)result.words[result.wordCount - 1]);
    
    if (wordBuf[0] == 'U') {  // Prüfe auf "UHR"
      hasUhr = 1;
    }
    DEBUG_PRINT(result.text);  // text[] liegt im RAM
    
    // Wörter durchgehen
    for (uint8_t i = 0; i < result.wordCount; i++) {
      // Wort aus PROGMEM in Buffer kopieren
      strcpy_P(wordBuf, (PGM_P)result.words[i]);
      
      // Bevorzugt exakt das Vorkommen malen, das der Validator geprueft hat
      // (sonst kann z.B. ein anderes "ACHT"/"ZEHN" im Pattern leuchten).
      if (result.positions[i] >= 0)
      {
        setWordAtAuto(result.positions[i], strlen(wordBuf));
      }
      // Hervorhebung für Stundenwort (letztes vor UHR)
      else if (i == (result.wordCount - 1 - hasUhr))
      {
        setWordAuto(wordBuf, 0, true);
      }
      else
      {
        setWordAuto(wordBuf, 0);
      }
    }

    //Minuten
    for (int i = 0; i < 4; i++)
    {
        if(result.ledHex & (1 << i))
        {
          leds[bridgeLED(MINUTE_LEDS[3-i])] = colorForLed(MINUTE_LEDS[3-i]);
        }
        else
        {
          leds[bridgeLED(MINUTE_LEDS[3-i])] = CRGB::Black;
        }
        
        if(result.ledHex & (1 << (3-i)))
        {
          
          DEBUG_PRINT(" ●");
        }
        else
        {
          DEBUG_PRINT(" ○");
        }
    }
    DEBUG_PRINT("\n");
  }
  else
  {
    DEBUG_PRINTF("ERROR %d",resultval);
  }
  yield();
  noInterrupts();
  showLEDs(); // finaler Frame
  interrupts();
  yield();
  //fadeInCurrentFrame(80,200,15);
}

// ════════════════════════════════════════════════════════════════
// ZEITZONE & OTA FUNKTIONEN
// ════════════════════════════════════════════════════════════════
bool isDST(time_t utcTime) {
    struct tm* timeinfo = gmtime(&utcTime);
    int year = timeinfo->tm_year + 1900;
    int month = timeinfo->tm_mon + 1;  // 1-12
    int day = timeinfo->tm_mday;
    int hour = timeinfo->tm_hour;

    // Berechne letzten Sonntag im März (effizient)
    // Wochentag des 31. März berechnen, dann rückwärts zum Sonntag
    struct tm march31 = {0};
    march31.tm_year = year - 1900;
    march31.tm_mon = 2;  // März = 2
    march31.tm_mday = 31;
    mktime(&march31);
    int marchLastSunday = 31 - march31.tm_wday;  // Sonntag ist 0

    // Berechne letzten Sonntag im Oktober (effizient)
    struct tm oct31 = {0};
    oct31.tm_year = year - 1900;
    oct31.tm_mon = 9;  // Oktober = 9
    oct31.tm_mday = 31;
    mktime(&oct31);
    int octoberLastSunday = 31 - oct31.tm_wday;

    // Prüfe ob Sommerzeit aktiv
    if (month < 3 || month > 10) {
        return false;  // Januar, Februar, November, Dezember: Winterzeit
    }
    if (month > 3 && month < 10) {
        return true;   // April - September: Sommerzeit
    }

    // März: Sommerzeit ab letztem Sonntag 01:00 UTC
    if (month == 3) {
        if (day < marchLastSunday) return false;
        if (day > marchLastSunday) return true;
        if (hour < 1) return false;  // Vor 01:00 UTC
        return true;  // Ab 01:00 UTC
    }

    // Oktober: Winterzeit ab letztem Sonntag 01:00 UTC
    if (month == 10) {
        if (day < octoberLastSunday) return true;
        if (day > octoberLastSunday) return false;
        if (hour < 1) return true;  // Vor 01:00 UTC
        return false;  // Ab 01:00 UTC
    }

    return false;
}

void showOTAProgress(int progress, bool isError = false, bool isSuccess = false) {
    int ledCount = map(progress, 0, 100, 0, 110);

    FastLED.clear();

    CRGB color;
    if (isSuccess) {
        color = CRGB::Green;
        ledCount = 110;
    } else if (isError) {
        color = CRGB::Red;
        ledCount = 110;
    } else {
        color = CRGB::Blue;
    }

    for (int i = 0; i < ledCount && i < NUM_LEDS; i++) {
        int row = i / COLS;
        int col = i % COLS;
        int ledIndex;

        if (row % 2 == 0) {
            ledIndex = row * COLS + col;
        } else {
            ledIndex = row * COLS + (COLS - 1 - col);
        }

        if (ledIndex == 2) continue;  // LED 2 = Fotowiderstand, bleibt immer aus
        leds[bridgeLED(ledIndex)] = color;
    }

    int minuteLEDsToShow = progress / 25;
    for (int i = 0; i < 4 && i < minuteLEDsToShow; i++) {
        leds[bridgeLED(MINUTE_LEDS[i])] = color;
    }

    FastLED.setBrightness(80);
    showLEDs();
    yield();
}

void otaProgressCallback(size_t current, size_t total) {
    if (total > 0) {
        int progress = (current * 100) / total;
        otaProgress = progress;

        static int lastProgress = -1;
        if (progress != lastProgress) {
            showOTAProgress(progress);
            lastProgress = progress;

            DEBUG_PRINTF("OTA Progress: %d%% (%u/%u bytes)\n",
                        progress, current, total);
        }
    }
    yield();
}

// ════════════════════════════════════════════════════════════════
// FUNKTIONEN: ZEIT
// ════════════════════════════════════════════════════════════════
// Aktuelle Zeit in UTC (Sekunden): bootTime + Laufzeit minus ESP-Drift.
// Ohne frische NTP-Zeit wird stuendlich auf die driftkorrigierte RTC
// nachgezogen (frueher nur alle 24 h und erst ab 30 s Abweichung).
unsigned long nowUtc() {
    unsigned long up = uptimeSeconds();
    unsigned long cur = bootTime + up - espDriftCorrection();

    static unsigned long lastRtcCheckUp = 0;
    if (rtcPresent && up - lastRtcCheckUp >= 3600) {
        lastRtcCheckUp = up;
        bool ntpFresh = ntpSyncSuccessful && lastNtpSync > 0 &&
                        cur >= lastNtpSync && cur - lastNtpSync < 7200;
        if (!ntpFresh) {
            unsigned long rtcTime = getCorrectedRTCTime();
            if (rtcTime > 1735689600UL) {
                long diff = (long)(cur - rtcTime);
                if (labs(diff) >= 2) {
                    DEBUG_PRINTF("RTC-Abgleich: %ld s\n", diff);
                    anchorTime(rtcTime, false);
                    cur = rtcTime;
                }
            }
        }
    }
    return cur;
}

void getCurrentTime(int &hours, int &minutes, int &seconds) {
    unsigned long currentSeconds = nowUtc();

    // ════════════════════════════════════════════════════════════════
    // ZEITZONE: UTC → MEZ/MESZ (Deutschland)
    // ════════════════════════════════════════════════════════════════
    // currentSeconds ist aktuell in UTC
    // Prüfe ob Sommerzeit (MESZ) oder Winterzeit (MEZ)
    time_t utcTime = currentSeconds;

    //if(ntpEnabled == true)
    {
      if (isDST(utcTime)) {
        // MESZ = UTC+2
        currentSeconds += 7200;  // +2 Stunden
      } else {
        // MEZ = UTC+1
        currentSeconds += 3600;  // +1 Stunde
      }
    }
    seconds = currentSeconds % 60;
    minutes = (currentSeconds / 60) % 60;
    hours = (currentSeconds / 3600) % 24;
}

// Zeichnet die aktuelle Zeit sofort neu (nach Einstellungsaenderungen) und
// beruecksichtigt dabei den Sonderwort-Modus – ein nacktes displayTime()
// wuerde im Parallel-Modus die Sonderwoerter bis zur naechsten Minute loeschen.
void renderCurrentTime() {
    if (powerLossDetected) return;
    int hours, minutes, seconds;
    getCurrentTime(hours, minutes, seconds);
    if (specialWordMode == SPECIAL_WORD_MODE_PARALLEL) {
        displayTimeWithSpecial(hours, minutes);
    } else {
        displayTime(hours, minutes);
    }
    lastDisplayedMinute = minutes;
    lastUpdateTime = millis();
}

// ════════════════════════════════════════════════════════════════
// WEBSERVER HANDLER
// ════════════════════════════════════════════════════════════════
// Maskiert einen String fuer die Ausgabe in JSON. SSIDs (auch fremde aus dem
// Scan), Identities, Hostnamen und Sonderwoerter koennen " oder \ enthalten –
// unmaskiert bricht JSON.parse im Browser und die ganze Seite/Liste faellt aus.
String jsonEscape(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((uint8_t)c < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void handleRoot() {
    DEBUG_PRINTLN("→ handleRoot aufgerufen");

    // GZIP-komprimierte HTML-Seite senden
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html", (const char*)HTML_PAGE_GZIP, HTML_PAGE_GZIP_LEN);

    DEBUG_PRINTF("  HTML gesendet: %u Bytes (komprimiert)\n", (unsigned)HTML_PAGE_GZIP_LEN);
}

void handleGetColors() {
    String json = "{";
    json += "\"nr\":" + String(normalColor.r) + ",";
    json += "\"ng\":" + String(normalColor.g) + ",";
    json += "\"nb\":" + String(normalColor.b) + ",";
    json += "\"sr\":" + String(specialColor.r) + ",";
    json += "\"sg\":" + String(specialColor.g) + ",";
    json += "\"sb\":" + String(specialColor.b) + ",";
    json += "\"specialBrightness\":" + String(specialBrightness) + ",";
    json += "\"brightness\":" + String(brightness) + ",";
    json += "\"rainbow\":" + String(useRainbow ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
}

void handleGetTime() {
    DEBUG_PRINTLN("→ handleGetTime aufgerufen");
    int h, m, s;
    getCurrentTime(h, m, s);

    // UTC-Zeit berechnen (identisch zur Anzeige inkl. Driftkorrektur)
    unsigned long currentSecondsUTC = nowUtc();

    // Zeitzonenkorrektur: UTC → MEZ/MESZ (wie in getCurrentTime)
    time_t utcTime = currentSecondsUTC;
    unsigned long currentSecondsLocal = currentSecondsUTC;

    if (isDST(utcTime)) {
        // MESZ = UTC+2
        currentSecondsLocal += 7200;  // +2 Stunden
    } else {
        // MEZ = UTC+1
        currentSecondsLocal += 3600;  // +1 Stunde
    }

    String json = "{";
    json += "\"time\":\"" + String(h) + ":" + String(m) + ":" + String(s) + "\",";
    json += "\"timestamp\":" + String(currentSecondsLocal) + ",";  // Jetzt lokale Zeit (MEZ/MESZ)
    json += "\"lastSync\":" + String(lastSyncTime) + ",";
    json += "\"driftRate\":" + String(driftRate, 3) + ",";
    json += "\"syncCount\":" + String(syncCount) + ",";
    json += "\"uptime\":" + String(uptimeSeconds()) + ",";
    json += "\"powerLoss\":" + String(powerLossDetected ? "true" : "false") + ",";
    json += "\"rtcPresent\":" + String(rtcPresent ? "true" : "false") + ",";
    json += "\"rtcDriftRate\":" + String(rtcDriftRate, 3) + ",";
    json += "\"rtcDriftLocked\":" + String(rtcDriftLocked ? "true" : "false") + ",";
    // Aufsummierte Messdauer der Drift-Kalibrierung in Stunden
    json += "\"rtcCalibHours\":" + String(rtcCalTimeSum / 3600.0f, 1);
    json += "}";

    server.send(200, "application/json", json);
}

void handleRTCDriftLock() {
    if (server.hasArg("locked")) {
        rtcDriftLocked = (server.arg("locked") == "1" || server.arg("locked") == "true");
        saveRTCDrift();
    }
    server.send(200, "application/json",
                String("{\"rtcDriftLocked\":") + (rtcDriftLocked ? "true" : "false") + "}");
}

void handleRTCDriftReset() {
    // Messung neu beginnen. Den Referenzpunkt (letzter bekannter Versatz)
    // behalten – der ist weiterhin gueltig und wird fuer die Korrektur gebraucht.
    rtcDriftRate = 0.0f;
    rtcDriftLocked = false;
    rtcCalDriftSum = 0.0f;
    rtcCalTimeSum = 0;
    saveRTCDrift();
    server.send(200, "text/plain", "OK - RTC-Drift zurueckgesetzt");
}

void clearPowerLossWarning();  // Forward-Declaration, Definition weiter unten

void handlePowerLossClear() {
    DEBUG_PRINTLN("→ handlePowerLossClear aufgerufen");
    clearPowerLossWarning();
    server.send(200, "text/plain", "OK - Stromausfall-Warnung quittiert");
}

// ════════════════════════════════════════════════════════════════
// PATTERN TEST HANDLER
// ════════════════════════════════════════════════════════════════
// Kritische Test-Zeiten: Zeiten mit Wortueberlappungen (EIN/EINS, VIER in
// VIERTEL, FUENF/ZEHN doppelt).
static const int PATTERN_TEST_TIMES[][2] = {
    // Stunde 0 und 1 KOMPLETT (EIN vs EINS, mit/ohne UHR)
    {0, 0}, {0, 5}, {0, 10}, {0, 15}, {0, 20}, {0, 25}, {0, 30}, {0, 35}, {0, 40}, {0, 45}, {0, 50}, {0, 55},
    {1, 0}, {1, 5}, {1, 10}, {1, 15}, {1, 20}, {1, 25}, {1, 30}, {1, 35}, {1, 40}, {1, 45}, {1, 50}, {1, 55},
    // Beispiel weitere Stunden
    {12, 0}, {12, 25},  // ZWÖLF und HALB EINS
    // VIER als Stunde (weil "VIER" auch in VIERTEL steckt)
    {3, 45},  // VIERTEL VOR VIER
    {4, 0}, {4, 5}, {4, 10}, {4, 15}, {4, 20}, {4, 25}, {4, 30}, {4, 35}, {4, 40}, {4, 45}, {4, 50}, {4, 55},
    // FÜNF als Stunde (weil "FuNF" zweimal vorkommt)
    {5, 0}, {5, 5}, {5, 10}, {5, 15}, {5, 20}, {5, 25}, {5, 30}, {5, 35}, {5, 40}, {5, 45}, {5, 50}, {5, 55},
    // ZEHN als Stunde (weil "ZEHN" zweimal vorkommt)
    {9, 50},  // ZEHN VOR ZEHN
    {10, 0}, {10, 5}, {10, 10}, {10, 15}, {10, 20}, {10, 25}, {10, 30}, {10, 35}, {10, 40}, {10, 45}, {10, 50}, {10, 55}
};
static const int PATTERN_TEST_COUNT = sizeof(PATTERN_TEST_TIMES) / sizeof(PATTERN_TEST_TIMES[0]);
static int patternTestIndex = 0;
static unsigned long patternTestNextMs = 0;

// Startet den Pattern-Test. Der eigentliche Ablauf passiert schrittweise in
// runPatternTestStep() aus loop() – frueher blockierte der Handler ~2 min
// und rief den Webserver rekursiv auf, was Requests durcheinanderbringen kann.
void handlePatternTest() {
    DEBUG_PRINTLN("→ handlePatternTest aufgerufen");

    if (patternTestRunning) {
        server.send(409, "text/plain", "Pattern-Test läuft bereits!");
        return;
    }

    server.send(200, "text/plain", "Pattern-Test gestartet - Nur kritische Zeiten werden getestet");
    patternTestRunning = true;
    patternTestIndex = 0;
    patternTestNextMs = millis();
}

void runPatternTestStep() {
    if (!patternTestRunning || (long)(millis() - patternTestNextMs) < 0) return;

    if (patternTestIndex >= PATTERN_TEST_COUNT) {
        patternTestRunning = false;
        DEBUG_PRINTLN("✓ Pattern-Test abgeschlossen");
        renderCurrentTime();  // zurueck zur aktuellen Zeit
        return;
    }
    int h = PATTERN_TEST_TIMES[patternTestIndex][0];
    int m = PATTERN_TEST_TIMES[patternTestIndex][1];
    DEBUG_PRINTF("Pattern-Test: %02d:%02d (%d/%d)\n", h, m, patternTestIndex + 1, PATTERN_TEST_COUNT);
    displayTime(h, m);
    patternTestIndex++;
    patternTestNextMs = millis() + 2000;
}

void handleGetCharsoap() {
    DEBUG_PRINTLN("→ handleGetCharsoap aufgerufen");
    server.send(200, "text/plain", charsoap);
}

void handleResetCharsoap() {
    DEBUG_PRINTLN("→ handleResetCharsoap aufgerufen");
    resetCharsoap();
    server.send(200, "text/plain", "OK");
}

// ════════════════════════════════════════════════════════════════
// SPECIAL WORD HANDLER
// ════════════════════════════════════════════════════════════════
void handleGetSpecialWords() {
    DEBUG_PRINTLN("→ handleGetSpecialWords aufgerufen");

    String json = "{\"words\":[";
    for (int i = 0; i < MAXWORDS; i++) {
        json += "\"";
        json += jsonEscape(String(SPECIAL_WORD[i]));
        json += "\"";
        if (i < MAXWORDS - 1) json += ",";
    }
    json += "],\"interval\":";
    json += String(specialWordInterval);
    json += ",\"mode\":";
    json += String(specialWordMode);
    json += "}";

    server.send(200, "application/json", json);
}

void handleSaveSpecialWords() {
    DEBUG_PRINTLN("→ handleSaveSpecialWords aufgerufen");

    char newWords[MAXWORDS][12];

    for (int i = 0; i < MAXWORDS; i++) {
        String argName = "word" + String(i);
        if (server.hasArg(argName)) {
            String word = server.arg(argName);
            if (word.length() > 11) {
                server.send(400, "text/plain", "Wort " + String(i) + " zu lang (max 11 Zeichen)");
                return;
            }
            word.toCharArray(newWords[i], 12);
        } else {
            newWords[i][0] = '\0';
        }
    }

    // Intervall speichern (falls angegeben)
    if (server.hasArg("interval")) {
        uint8_t interval = server.arg("interval").toInt();
        saveSpecialWordInterval(interval);
    }

    // Modus speichern (falls angegeben)
    if (server.hasArg("mode")) {
        uint8_t mode = server.arg("mode").toInt();
        saveSpecialWordMode(mode);
    }

    saveSpecialWords(newWords);
    server.send(200, "text/plain", "OK - Spezialwörter gespeichert!");
}

void handleResetSpecialWords() {
    DEBUG_PRINTLN("→ handleResetSpecialWords aufgerufen");
    resetSpecialWords();
    resetSpecialWordInterval();
    server.send(200, "text/plain", "OK - Spezialwörter zurückgesetzt!");
}

// ════════════════════════════════════════════════════════════════
// MINUTE LEDS HANDLER
// ════════════════════════════════════════════════════════════════
void handleGetMinuteLeds() {
    DEBUG_PRINTLN("→ handleGetMinuteLeds aufgerufen");

    String json = "{\"leds\":[";
    for (int i = 0; i < 4; i++) {
        json += String(MINUTE_LEDS[i]);
        if (i < 3) json += ",";
    }
    json += "]}";

    server.send(200, "application/json", json);
}

void handleSaveMinuteLeds() {
    DEBUG_PRINTLN("→ handleSaveMinuteLeds aufgerufen");

    uint8_t newLeds[4];

    for (int i = 0; i < 4; i++) {
        String argName = "led" + String(i);
        if (!server.hasArg(argName)) {
            server.send(400, "text/plain", "LED " + String(i) + " fehlt");
            return;
        }

        int value = server.arg(argName).toInt();
        if (value < 0 || value >= NUM_LEDS) {
            server.send(400, "text/plain", "LED " + String(i) + " ungültig (0-" + String(NUM_LEDS-1) + ")");
            return;
        }

        newLeds[i] = value;
    }

    saveMinuteLeds(newLeds);
    server.send(200, "text/plain", "OK - Minuten-LEDs gespeichert!");
}

void handleResetMinuteLeds() {
    DEBUG_PRINTLN("→ handleResetMinuteLeds aufgerufen");
    resetMinuteLeds();
    server.send(200, "text/plain", "OK - Minuten-LEDs zurückgesetzt!");
}

void handleSave() {
    DEBUG_PRINTLN("→ handleSave aufgerufen");
    if (server.hasArg("timestamp")) {
        DEBUG_PRINT("handleSave");
        unsigned long utcTimestamp = strtoul(server.arg("timestamp").c_str(), nullptr, 10);
        long timezoneOffset = server.hasArg("tzoffset") ? server.arg("tzoffset").toInt() : 0;
        unsigned long clientTime = utcTimestamp - timezoneOffset;

        // Charsoap vorab pruefen, damit ein ungueltiges Pattern nicht still
        // mit "OK" quittiert wird.
        if (server.hasArg("charsoap") && server.arg("charsoap").length() > 0 &&
            server.arg("charsoap").length() != CHARSOAP_LEN) {
            server.send(400, "text/plain", "Wortmatrix muss genau " + String(CHARSOAP_LEN) +
                        " Zeichen haben (erhalten: " + String(server.arg("charsoap").length()) + ")");
            return;
        }

        // timestamp=0 bedeutet "Zeit nicht aendern" – wird vom Frontend bei
        // saveColors() / saveCharsoap() gesendet, um Drift-Berechnung, bootTime
        // und RTC nicht zu zerschiessen.
        if (utcTimestamp != 0) {
            if (clientTime < 1735689600UL) {
                server.send(400, "text/plain", "Ungueltige Zeit");
                return;
            }
            // ESP-Drift seit dem letzten praezisen Anker dieser Session messen
            // (Browserzeit ist auf ~1 s genau -> mind. 12 h Abstand verlangen)
            measureEspDrift((int64_t)clientTime * 1000, 12UL * 3600);
            anchorTime(clientTime, true);
            lastSyncTime = clientTime;
            saveDriftRate();
            rtcSetManually(clientTime);
        }

        // Farben/Helligkeit nur uebernehmen, wenn sie mitgeschickt wurden –
        // sonst wuerden fehlende Parameter als 0 (schwarz / dunkel) gespeichert.
        if (server.hasArg("nr") && server.hasArg("ng") && server.hasArg("nb")) {
            normalColor.r = server.arg("nr").toInt();
            normalColor.g = server.arg("ng").toInt();
            normalColor.b = server.arg("nb").toInt();
        }
        if (server.hasArg("rainbow")) {
            useRainbow = (server.arg("rainbow") == "1" || server.arg("rainbow") == "true");
        }
        if (server.hasArg("sr") && server.hasArg("sg") && server.hasArg("sb")) {
            specialColor.r = server.arg("sr").toInt();
            specialColor.g = server.arg("sg").toInt();
            specialColor.b = server.arg("sb").toInt();
        }
        if (server.hasArg("specialBrightness")) {
            int sb = server.arg("specialBrightness").toInt();
            if (sb < 0) sb = 0;
            if (sb > 100) sb = 100;
            specialBrightness = (uint8_t)sb;
        }
        if (server.hasArg("brightness")) {
            int b = server.arg("brightness").toInt();
            brightness = (uint8_t)constrain(b, 10, 80);
        }

        if (server.hasArg("charsoap") && server.arg("charsoap").length() == CHARSOAP_LEN) {
            // Achtung: nicht .toUpperCase() – Umlaute werden vom Frontend bereits
            // in Kleinbuchstaben (a/o/u) umgewandelt; ein zweites Uppercasen wuerde
            // diese Information verlieren.
            saveCharsoap(server.arg("charsoap").c_str());
        }

        saveConfig();
        // User-Helligkeit (0-80) auf LED-Helligkeit (0-204) mappen. Bei aktiver
        // Auto-Helligkeit uebernimmt updateBrightness() gleich wieder.
        FastLED.setBrightness(map(brightness, 0, 80, 0, 204));

        server.send(200, "text/plain", "OK");
        renderCurrentTime();
    } else {
        server.send(400, "text/plain", "Fehler");
    }
}

// ════════════════════════════════════════════════════════════════
// LED TEST HANDLER
// ════════════════════════════════════════════════════════════════
void handleLEDTest() {
    Serial.println("→ handleLEDTest aufgerufen");
    
    if (!server.hasArg("mode")) {
        server.send(400, "text/plain", "Fehler: mode fehlt");
        return;
    }
    
    String mode = server.arg("mode");
    Serial.printf("  Mode: %s\n", mode.c_str());
    
    // Clear-Modus
    if (mode == "clear") {
        FastLED.clear();
        showLEDs();
        Serial.println("  → Alle LEDs ausgeschaltet");
        server.send(200, "text/plain", "OK - LEDs ausgeschaltet");
        return;
    }
    
    // Farbe und Helligkeit auslesen
    uint8_t r = server.hasArg("r") ? server.arg("r").toInt() : 255;
    uint8_t g = server.hasArg("g") ? server.arg("g").toInt() : 255;
    uint8_t b = server.hasArg("b") ? server.arg("b").toInt() : 255;
    int testBrightness = server.hasArg("brightness") ? server.arg("brightness").toInt() : 80;
    testBrightness = constrain(testBrightness, 0, 204);  // 80 %-Obergrenze wie im Normalbetrieb

    CRGB color = CRGB(r, g, b);

    // Aktuelle LED-Helligkeit sichern (0-255-Skala – frueher wurde der
    // 0-80-User-Wert "zurueckgesetzt", die Uhr lief danach mit ~30 %) und bei
    // JEDEM Verlassen der Funktion wiederherstellen, auch bei Fehlern.
    struct BrightnessGuard {
        uint8_t saved;
        BrightnessGuard() : saved(FastLED.getBrightness()) {}
        ~BrightnessGuard() { FastLED.setBrightness(saved); }
    } brightnessGuard;
    FastLED.setBrightness(testBrightness);
    
    FastLED.clear();
    
    // Einzelne LED
    if (mode == "single") {
        if (!server.hasArg("led")) {
            server.send(400, "text/plain", "Fehler: led fehlt");
            return;
        }

        int ledNum = server.arg("led").toInt();

        if (ledNum < 0 || ledNum >= NUM_LEDS) {
            server.send(400, "text/plain", "Fehler: LED-Nummer ungültig");
            return;
        }

        // LED Nr. 2 darf NIEMALS eingeschaltet werden (Fotowiderstand)
        if (ledNum == 2) {
            Serial.println("  ⚠ LED 2 blockiert (Fotowiderstand)");
            server.send(400, "text/plain", "Fehler: LED 2 ist gesperrt (Fotowiderstand)");
            return;
        }

        leds[bridgeLED(ledNum)] = color;
        showLEDs();

        Serial.printf("  → LED %d eingeschaltet (R:%d G:%d B:%d)\n", ledNum, r, g, b);
        server.send(200, "text/plain", "OK - LED " + String(ledNum));
    }
    
    // LED-Bereich
    else if (mode == "range") {
        if (!server.hasArg("from") || !server.hasArg("to")) {
            server.send(400, "text/plain", "Fehler: from/to fehlt");
            return;
        }

        int from = server.arg("from").toInt();
        int to = server.arg("to").toInt();

        if (from < 0 || to >= NUM_LEDS || from > to) {
            server.send(400, "text/plain", "Fehler: Bereich ungültig");
            return;
        }

        for (int i = from; i <= to; i++) {
            // LED Nr. 2 muss IMMER aus sein (Fotowiderstand)
            if (i == 2) {
                leds[bridgeLED(i)] = CRGB::Black;
            } else {
                leds[bridgeLED(i)] = color;
            }
        }
        showLEDs();

        Serial.printf("  → LEDs %d-%d eingeschaltet (LED 2 bleibt aus)\n", from, to);
        server.send(200, "text/plain", "OK - LEDs " + String(from) + "-" + String(to));
    }
    
    // Alle LEDs (außer LED Nr. 2 - Fotowiderstand!)
    else if (mode == "all") {
        for (int i = 0; i < NUM_LEDS; i++) {
            // LED Nr. 2 muss IMMER aus sein (Fotowiderstand)
            if (i == 2) {
                leds[bridgeLED(i)] = CRGB::Black;
            } else {
                leds[bridgeLED(i)] = color;
            }
        }
        showLEDs();

        Serial.println("  → Alle LEDs eingeschaltet (außer LED 2)");
        server.send(200, "text/plain", "OK - Alle LEDs (außer LED 2)");
    }
    
    // Zeile
    else if (mode == "row") {
        if (!server.hasArg("row")) {
            server.send(400, "text/plain", "Fehler: row fehlt");
            return;
        }

        int row = server.arg("row").toInt();

        if (row < 0 || row >= ROWS) {
            server.send(400, "text/plain", "Fehler: Zeile ungültig");
            return;
        }

        for (int col = 0; col < COLS; col++) {
            int index;
            if (row % 2 == 0) {
                index = row * COLS + col;
            } else {
                index = row * COLS + (COLS - 1 - col);
            }
            // LED Nr. 2 muss IMMER aus sein (Fotowiderstand)
            if (index == 2) {
                leds[bridgeLED(index)] = CRGB::Black;
            } else {
                leds[bridgeLED(index)] = color;
            }
        }
        showLEDs();

        Serial.printf("  → Zeile %d eingeschaltet (LED 2 bleibt aus)\n", row);
        server.send(200, "text/plain", "OK - Zeile " + String(row));
    }

    // Spalte
    else if (mode == "col") {
        if (!server.hasArg("col")) {
            server.send(400, "text/plain", "Fehler: col fehlt");
            return;
        }

        int col = server.arg("col").toInt();

        if (col < 0 || col >= COLS) {
            server.send(400, "text/plain", "Fehler: Spalte ungültig");
            return;
        }

        for (int row = 0; row < ROWS; row++) {
            int index;
            if (row % 2 == 0) {
                index = row * COLS + col;
            } else {
                index = row * COLS + (COLS - 1 - col);
            }
            // LED Nr. 2 muss IMMER aus sein (Fotowiderstand)
            if (index == 2) {
                leds[bridgeLED(index)] = CRGB::Black;
            } else {
                leds[bridgeLED(index)] = color;
            }
        }
        showLEDs();
        
        Serial.printf("  → Spalte %d eingeschaltet\n", col);
        server.send(200, "text/plain", "OK - Spalte " + String(col));
    }
    
    else {
        server.send(400, "text/plain", "Fehler: Unbekannter Modus");
    }
    // Helligkeit wird durch brightnessGuard wiederhergestellt
}

// ════════════════════════════════════════════════════════════════
// OTA UPDATE HANDLER
// ════════════════════════════════════════════════════════════════
// Verzoegerter Neustart: erst die HTTP-Antwort rausschicken, dann aus loop()
// neu starten (frueher startete das ESP im Upload-Handler neu, der Browser
// bekam keine Antwort und meldete "Netzwerkfehler" trotz Erfolg).
static unsigned long restartAtMs = 0;
void scheduleRestart(unsigned long delayMs) {
    restartAtMs = millis() + delayMs;
    if (restartAtMs == 0) restartAtMs = 1;
}
void handlePendingRestart() {
    if (restartAtMs != 0 && (long)(millis() - restartAtMs) >= 0) {
        ESP.restart();
    }
}

void handleOTAInfo() {
    DEBUG_PRINTLN("→ handleOTAInfo aufgerufen");

    String json = "{";
    json += "\"version\":\"" + String(firmwareVersion) + "\",";
    json += "\"buildDate\":\"" + String(BUILD_DATE) + "\",";
    json += "\"buildTime\":\"" + String(BUILD_TIME) + "\",";
    json += "\"freeSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
    json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
    json += "\"chipId\":\"" + String(ESP.getChipId(), HEX) + "\",";
    json += "\"flashSize\":" + String(ESP.getFlashChipRealSize()) + ",";
    json += "\"otaReady\":" + String(otaInProgress ? "false" : "true");
    json += "}";

    server.send(200, "application/json", json);
}

void handleOTAUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        otaInProgress = true;
        otaProgress = 0;
        otaError = "";
        otaStartTime = millis();

        DEBUG_PRINTF("OTA Upload Start: %s\n", upload.filename.c_str());

        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        showOTAProgress(0);

        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
            Update.printError(Serial);
            otaError = "Begin failed";
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (otaError.length() > 0) return;  // Begin/Write bereits fehlgeschlagen
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
            otaError = "Write failed";
        } else {
            size_t progress = Update.progress();
            size_t total = Update.size();
            if (total > 0) {
                otaProgressCallback(progress, total);
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (otaError.length() == 0 && Update.end(true)) {
            DEBUG_PRINTF("OTA Success: %u bytes\n", upload.totalSize);
            showOTAProgress(100, false, true);

            EEPROM.write(ADDR_OTA_FLAGS, OTA_FLAG_UPDATE_SUCCESS);
            saveLastKnownTime(false);
            EEPROM.commit();

            otaInProgress = false;
            // Neustart erst NACH der Antwort (handleOTAUploadDone -> loop())
        } else {
            if (otaError.length() == 0) {
                Update.printError(Serial);
                otaError = String(Update.getError());
            }
            showOTAProgress(100, true, false);
            otaInProgress = false;
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
        // Browser hat den Upload abgebrochen – Update verwerfen, sonst bliebe
        // otaInProgress bis zum Neustart haengen.
        Update.end(false);
        otaError = "Upload abgebrochen";
        showOTAProgress(100, true, false);
        otaInProgress = false;
    }
}

void handleOTAUploadDone() {
    if (otaError.length() > 0) {
        String json = "{\"success\":false,\"message\":\"" + jsonEscape(otaError) + "\"}";
        server.send(500, "application/json", json);
    } else {
        String json = "{\"success\":true,\"message\":\"Update erfolgreich - Neustart\"}";
        server.send(200, "application/json", json);
        scheduleRestart(3000);
    }
}

void handleOTAFromURL() {
    DEBUG_PRINTLN("→ handleOTAFromURL aufgerufen");

    if (!server.hasArg("url")) {
        server.send(400, "application/json",
                   "{\"success\":false,\"message\":\"URL fehlt\"}");
        return;
    }

    String firmwareURL = server.arg("url");

    if (firmwareURL.length() == 0) {
        server.send(400, "application/json",
                   "{\"success\":false,\"message\":\"URL leer\"}");
        return;
    }

    server.send(200, "application/json",
               "{\"success\":true,\"message\":\"Update gestartet\"}");

    delay(500);

    otaInProgress = true;
    otaProgress = 0;
    otaError = "";
    otaStartTime = millis();

    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    showOTAProgress(0);

    DEBUG_PRINTLN("Starting HTTP update from: " + firmwareURL);

    ESPhttpUpdate.onProgress([](int current, int total) {
        otaProgressCallback(current, total);
    });

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    // Selbst neu starten, damit vorher das OTA-Flag geschrieben wird – sonst
    // startet die Bibliothek sofort neu und der Neustart gilt als Stromausfall.
    ESPhttpUpdate.rebootOnUpdate(false);

    WiFiClient client;
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareURL);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            DEBUG_PRINTF("HTTP_UPDATE_FAILED Error (%d): %s\n",
                        ESPhttpUpdate.getLastError(),
                        ESPhttpUpdate.getLastErrorString().c_str());
            otaError = ESPhttpUpdate.getLastErrorString();
            showOTAProgress(100, true, false);
            otaInProgress = false;
            break;

        case HTTP_UPDATE_NO_UPDATES:
            DEBUG_PRINTLN("HTTP_UPDATE_NO_UPDATES");
            otaError = "No updates available";
            showOTAProgress(100, true, false);
            otaInProgress = false;
            break;

        case HTTP_UPDATE_OK:
            DEBUG_PRINTLN("HTTP_UPDATE_OK");
            showOTAProgress(100, false, true);

            EEPROM.write(ADDR_OTA_FLAGS, OTA_FLAG_UPDATE_SUCCESS);
            saveLastKnownTime(false);
            EEPROM.commit();

            otaInProgress = false;
            scheduleRestart(3000);
            break;
    }
}

void handleOTAStatus() {
    DEBUG_PRINTLN("→ handleOTAStatus aufgerufen");

    String json = "{";
    json += "\"inProgress\":" + String(otaInProgress ? "true" : "false") + ",";
    json += "\"progress\":" + String(otaProgress) + ",";
    json += "\"error\":\"" + jsonEscape(otaError) + "\",";
    json += "\"elapsed\":" + String(millis() - otaStartTime);
    json += "}";

    server.send(200, "application/json", json);
}

// ════════════════════════════════════════════════════════════════
// WIFI/NTP CONFIG HANDLER
// ════════════════════════════════════════════════════════════════
void handleGetWiFiConfig() {
    DEBUG_PRINTLN("→ handleGetWiFiConfig aufgerufen");

    String json = "{";

    // WiFi Station Config
    json += "\"staEnabled\":" + String(staEnabled ? "true" : "false") + ",";
    json += "\"staSsid\":\"" + jsonEscape(String(staSsid)) + "\",";
    // Passwort niemals im Klartext ausliefern: Frontend bekommt nur einen Marker,
    // dass eines gespeichert ist. Beim /wifi/save wird ein leeres Feld als
    // "bestehendes Passwort beibehalten" interpretiert (staPasswordKeep=1).
    json += "\"staPassword\":\"\",";
    json += "\"staPasswordSet\":" + String(staPassword[0] != '\0' ? "true" : "false") + ",";
    // WPA2-Enterprise (PEAP/MS-CHAPv2): Identity wird gespeichert und ausgeliefert,
    // sie ist nicht geheim (wird auf Funk-Ebene ohnehin sichtbar uebertragen).
    json += "\"staEnterprise\":" + String(staEnterprise ? "true" : "false") + ",";
    json += "\"staIdentity\":\"" + jsonEscape(String(staIdentity)) + "\",";
    json += "\"staAnonIdentity\":\"" + jsonEscape(String(staAnonIdentity)) + "\",";
    json += "\"staHostname\":\"" + jsonEscape(String(staHostname)) + "\",";
    json += "\"staDhcp\":" + String(staDhcp ? "true" : "false") + ",";
    json += "\"staIP\":\"" + staIP.toString() + "\",";
    json += "\"staGateway\":\"" + staGateway.toString() + "\",";
    json += "\"staSubnet\":\"" + staSubnet.toString() + "\",";
    json += "\"staDNS\":\"" + staDNS.toString() + "\",";

    // WiFi Status
    json += "\"staConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"staLocalIP\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"staRSSI\":" + String(WiFi.RSSI()) + ",";

    // NTP Config
    json += "\"ntpEnabled\":" + String(ntpEnabled ? "true" : "false") + ",";
    json += "\"ntpServer\":\"" + jsonEscape(String(ntpServer)) + "\",";
    json += "\"ntpLastSync\":" + String(lastNtpSync) + ",";
    json += "\"ntpSyncOk\":" + String(ntpSyncSuccessful ? "true" : "false");

    json += "}";

    server.send(200, "application/json", json);
}

void handleSaveWiFiConfig() {
    DEBUG_PRINTLN("→ handleSaveWiFiConfig aufgerufen");

    bool changed = false;

    // WiFi Station Enabled
    if (server.hasArg("staEnabled")) {
        staEnabled = (server.arg("staEnabled") == "true" || server.arg("staEnabled") == "1");
        changed = true;
    }

    // SSID
    if (server.hasArg("staSsid")) {
        String newSsid = server.arg("staSsid");
        strncpy(staSsid, newSsid.c_str(), 31);
        staSsid[31] = '\0';
        changed = true;
    }

    // Password: leeres Feld + staPasswordKeep=1 = bestehendes Passwort behalten
    if (server.hasArg("staPassword")) {
        String newPassword = server.arg("staPassword");
        bool keep = server.hasArg("staPasswordKeep") &&
                    (server.arg("staPasswordKeep") == "1" || server.arg("staPasswordKeep") == "true");
        if (!(keep && newPassword.length() == 0)) {
            strncpy(staPassword, newPassword.c_str(), 63);
            staPassword[63] = '\0';
            changed = true;
        }
    }

    // WPA2-Enterprise
    if (server.hasArg("staEnterprise")) {
        staEnterprise = (server.arg("staEnterprise") == "true" || server.arg("staEnterprise") == "1");
        changed = true;
    }
    if (server.hasArg("staIdentity")) {
        String newIdentity = server.arg("staIdentity");
        strncpy(staIdentity, newIdentity.c_str(), 63);
        staIdentity[63] = '\0';
        changed = true;
    }
    if (server.hasArg("staAnonIdentity")) {
        String newAnon = server.arg("staAnonIdentity");
        strncpy(staAnonIdentity, newAnon.c_str(), 63);
        staAnonIdentity[63] = '\0';
        changed = true;
    }
    if (server.hasArg("staHostname")) {
        String newHost = server.arg("staHostname");
        strncpy(staHostname, newHost.c_str(), 63);
        staHostname[63] = '\0';
        changed = true;
    }

    // DHCP
    if (server.hasArg("staDhcp")) {
        staDhcp = (server.arg("staDhcp") == "true" || server.arg("staDhcp") == "1");
        changed = true;
    }

    // Statische IP-Konfiguration
    if (server.hasArg("staIP")) {
        staIP.fromString(server.arg("staIP"));
        changed = true;
    }
    if (server.hasArg("staGateway")) {
        staGateway.fromString(server.arg("staGateway"));
        changed = true;
    }
    if (server.hasArg("staSubnet")) {
        staSubnet.fromString(server.arg("staSubnet"));
        changed = true;
    }
    if (server.hasArg("staDNS")) {
        staDNS.fromString(server.arg("staDNS"));
        changed = true;
    }

    // NTP Enabled
    if (server.hasArg("ntpEnabled")) {
        ntpEnabled = (server.arg("ntpEnabled") == "true" || server.arg("ntpEnabled") == "1");
        changed = true;
    }

    // NTP Server (optional, Standard: ptbtime1.ptb.de)
    if (server.hasArg("ntpServer")) {
        String newNtpServer = server.arg("ntpServer");
        strncpy(ntpServer, newNtpServer.c_str(), 31);
        ntpServer[31] = '\0';
        changed = true;
    }

    if (changed) {
        saveWiFiStationConfig();
        saveNTPConfig();

        server.send(200, "text/plain", "OK - Neustart erforderlich!");

        // Info: Neustart nötig für WiFi-Änderungen
        DEBUG_PRINTLN("WiFi/NTP Config gespeichert - Neustart empfohlen");
    } else {
        server.send(400, "text/plain", "Keine Änderungen");
    }
}

void handleRestart() {
    DEBUG_PRINTLN("→ Neustart angefordert über Web-Interface");

    server.send(200, "text/plain", "Neustart wird durchgeführt...");

    // Gewollter Neustart ist kein Stromausfall: Zeit sichern und Running-Flag
    // loeschen (clearRunningFlag() wurde bisher nirgends aufgerufen).
    saveLastKnownTime(false);
    clearRunningFlag();
    scheduleRestart(500);  // Neustart aus loop(), nachdem die Antwort raus ist
}

void handleWiFiScan() {
    DEBUG_PRINTLN("→ handleWiFiScan aufgerufen");

    // Starte WiFi-Scan (synchron - kann 2-5 Sek. dauern!)
    int networksFound = WiFi.scanNetworks(false, false);
    yield();  // Watchdog füttern nach Scan

    if (networksFound == 0) {
        server.send(200, "application/json", "{\"networks\":[]}");
        return;
    }

    // JSON-Array erstellen
    String json = "{\"networks\":[";

    for (int i = 0; i < networksFound; i++) {
        yield();  // Watchdog füttern in Schleife
        if (i > 0) json += ",";

        json += "{";
        json += "\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }

    json += "]}";

    // Scan-Daten löschen (Speicher freigeben)
    WiFi.scanDelete();

    server.send(200, "application/json", json);
    DEBUG_PRINTF("✓ WiFi-Scan abgeschlossen: %d Netzwerke gefunden\n", networksFound);
}

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS CONFIG HANDLER
// ════════════════════════════════════════════════════════════════
void handleGetAutoBrightness() {
    //DEBUG_PRINTLN("→ handleGetAutoBrightness aufgerufen");

    // Aktuellen ADC-Wert auslesen (Mittelwert über 10 Messungen)
    uint32_t adcSum = 0;
    for (uint8_t i = 0; i < 10; i++) {
        adcSum += analogRead(A0);
        delay(1);
    }
    uint16_t currentADC = adcSum / 10;

    String json = "{";
    json += "\"enabled\":" + String(autoBrightnessEnabled ? "true" : "false") + ",";
    json += "\"minADC\":" + String(autoBrightnessMinADC) + ",";
    json += "\"maxADC\":" + String(autoBrightnessMaxADC) + ",";
    json += "\"minBrightness\":" + String(autoBrightnessMin) + ",";
    json += "\"maxBrightness\":" + String(autoBrightnessMax) + ",";
    json += "\"currentADC\":" + String(currentADC);
    json += "}";

    server.send(200, "application/json", json);
}

void handleSaveAutoBrightness() {
    DEBUG_PRINTLN("→ handleSaveAutoBrightness aufgerufen");

    bool changed = false;

    // Enabled Flag
    if (server.hasArg("enabled")) {
        autoBrightnessEnabled = (server.arg("enabled") == "true" || server.arg("enabled") == "1");
        changed = true;
    }

    // Min ADC Wert
    if (server.hasArg("minADC")) {
        int newMinADC = server.arg("minADC").toInt();
        // max. 1003, damit Max-ADC noch mindestens 20 darueber liegen kann
        // (bei Min == Max teilt map() durch 0)
        if (newMinADC >= 0 && newMinADC <= 1003) {
            autoBrightnessMinADC = newMinADC;
            // Auto-Korrektur: Max muss größer sein
            if (autoBrightnessMaxADC <= autoBrightnessMinADC) {
                autoBrightnessMaxADC = min(1023, autoBrightnessMinADC + 20);
            }
            changed = true;
        }
    }

    // Max ADC Wert
    if (server.hasArg("maxADC")) {
        int newMaxADC = server.arg("maxADC").toInt();
        if (newMaxADC >= 0 && newMaxADC <= 1023 && newMaxADC > (int)autoBrightnessMinADC) {
            autoBrightnessMaxADC = newMaxADC;
            changed = true;
        }
    }

    // Min Helligkeit
    if (server.hasArg("minBrightness")) {
        int newMinBrightness = server.arg("minBrightness").toInt();
        if (newMinBrightness >= 0 && newMinBrightness <= 80) {
            autoBrightnessMin = newMinBrightness;
            // Auto-Korrektur: Max muss größer sein
            if (autoBrightnessMax <= autoBrightnessMin) {
                autoBrightnessMax = min(80, autoBrightnessMin + 5);
            }
            changed = true;
        }
    }

    // Max Helligkeit
    if (server.hasArg("maxBrightness")) {
        int newMaxBrightness = server.arg("maxBrightness").toInt();
        // Max darf nicht > 80 sein
        if (newMaxBrightness >= 0 && newMaxBrightness <= 80) {
            autoBrightnessMax = newMaxBrightness;
            // Auto-Korrektur: Min muss kleiner sein
            if (autoBrightnessMin >= autoBrightnessMax) {
                autoBrightnessMin = max(0, autoBrightnessMax - 5);
            }
            changed = true;
        }
    }

    if (changed) {
        saveAutoBrightnessConfig();
        if (!autoBrightnessEnabled) {
            // Beim Abschalten sonst bliebe der letzte Auto-Wert stehen
            FastLED.setBrightness(map(brightness, 0, 80, 0, 204));
            showLEDs();
        }
        server.send(200, "text/plain", "OK - Auto-Brightness konfiguriert!");
        DEBUG_PRINTLN("Auto-Brightness Config gespeichert");
    } else {
        server.send(400, "text/plain", "Keine Änderungen oder ungültige Werte");
    }
}

void handleNotFound() {
    DEBUG_PRINTLN("→ handleNotFound aufgerufen");
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}
// ════════════════════════════════════════════════════════════════
// STROMAUSFALL-ERKENNUNG ohne RTC
// ════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════
// STROMAUSFALL-ERKENNUNG MIT RTC
// ════════════════════════════════════════════════════════════════
bool detectPowerLossWithRTC() {
    // Driftkorrigierte RTC-Zeit – sonst liefe die Uhr nach einem Neustart
    // ohne WLAN bis zum naechsten RTC-Abgleich mit der unkorrigierten Zeit.
    unsigned long rtcTime = getCorrectedRTCTime();

    // Hole letzte gespeicherte Zeit aus EEPROM
    unsigned long lastKnownTime = eepromReadU32(ADDR_TIMESTAMP);
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println(  "║    STROMAUSFALL-PRÜFUNG (RTC)          ║");
    Serial.println(  "╚════════════════════════════════════════╝");
    
    Serial.printf("RTC-Zeit:             %lu\n", rtcTime);
    Serial.printf("Letzte bekannte Zeit: %lu\n", lastKnownTime);
    
    // Prüfe ob gültige Zeit gespeichert ist (0xFFFFFFFF = frisches EEPROM)
    if (lastKnownTime < 1735689600UL || lastKnownTime > 4102444800UL) {
        Serial.println("→ Keine gültige Zeit gespeichert");
        Serial.println("✓ Erste Inbetriebnahme\n");

        // Übernehme RTC-Zeit als Startpunkt
        anchorTime(rtcTime, false);
        return false;
    }
    
    // RTC liegt VOR letzter bekannter Zeit → RTC-Batterie leer
    if (rtcTime < lastKnownTime) {
        long diff = lastKnownTime - rtcTime;
        Serial.printf("⚠⚠⚠ RTC liegt %ld Sekunden HINTER!\n", diff);
        Serial.println("⚠⚠⚠ STROMAUSFALL + RTC-BATTERIE LEER!");
        Serial.println("════════════════════════════════════════\n");
        return true;
    }
    
    // Prüfe Zeitdifferenz
    long timeDiff = rtcTime - lastKnownTime;
    Serial.printf("Zeitdifferenz: %ld Sek (%ld Tage)\n", timeDiff, timeDiff / 86400);
    
    // Wenn mehr als 7 Tage Unterschied → wahrscheinlich Stromausfall
    if (timeDiff > 604800) {  // 7 Tage in Sekunden
        Serial.println("⚠ Sehr lange Zeitdifferenz!");
        Serial.println("⚠ STROMAUSFALL erkannt");
        Serial.println("  (RTC lief weiter, ESP8266 war aus)");
        Serial.println("════════════════════════════════════════\n");
        
        // Übernehme RTC-Zeit
        anchorTime(rtcTime, false);
        return true;
    }
    
    // Alles OK - normale Zeitdifferenz
    Serial.println("✓ Normale Zeitdifferenz");
    Serial.println("✓ Kein Stromausfall\n");
    
    // Übernehme RTC-Zeit als Basis
    anchorTime(rtcTime, false);
    return false;
}

// Non-blocking SOS-Renderer (... --- ...) fuer den Stromausfall-Modus.
// Wird aus loop() statt displayTime() aufgerufen, solange powerLossDetected
// true ist. Der Webserver laeuft parallel weiter, der User kann die Zeit
// setzen und ueber /powerloss/clear quittieren.
void renderPowerLossSOS() {
    // Pattern: 3x kurz, Pause, 3x lang, Pause, 3x kurz, lange Pause
    static const uint16_t durations[] = {
        200,200, 200,200, 200,200, 400,   // S
        600,200, 600,200, 600,200, 400,   // O
        200,200, 200,200, 200,200, 2000   // S + lange Pause
    };
    static const bool   ledOn[] = {
        true,false, true,false, true,false, false,
        true,false, true,false, true,false, false,
        true,false, true,false, true,false, false
    };
    static const uint8_t SEQ_LEN = sizeof(durations) / sizeof(durations[0]);
    static unsigned long phaseStart = 0;
    static uint8_t phase = 0;
    static int8_t lastApplied = -1;  // -1 = noch nichts gerendert

    if (phaseStart == 0) phaseStart = millis();

    int8_t want = ledOn[phase] ? 1 : 0;
    if (want != lastApplied) {
        if (want) {
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(40);
        } else {
            FastLED.clear();
        }
        showLEDs();
        lastApplied = want;
    }

    if (millis() - phaseStart >= durations[phase]) {
        phaseStart = millis();
        phase = (phase + 1) % SEQ_LEN;
    }
}

// Wird vom Endpoint /powerloss/clear aufgerufen, sobald der User die
// Stromausfall-Warnung quittiert hat. Setzt den Marker zurueck, schreibt
// das Running-Flag (damit der naechste Boot nicht erneut anschlaegt) und
// stoesst eine sofortige Zeit-Anzeige an.
void clearPowerLossWarning() {
    powerLossDetected = false;
    setRunningFlag();
    FastLED.clear();
    showLEDs();
    lastDisplayedMinute = -1;  // erzwingt Neu-Render im naechsten loop()
}

void powerLossLoop() {
    Serial.println("⚠ STROMAUSFALL-WARNUNG - Blinke SOS-Muster");

    while (true) {
        // SOS-Muster: ... --- ...
        
        // S (3× kurz)
        for (int i = 0; i < 3; i++) {
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(40);
            showLEDs();
            delay(200);
            FastLED.clear();
            showLEDs();
            delay(200);
        }
        
        delay(400);
        
        // O (3× lang)
        for (int i = 0; i < 3; i++) {
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(40);
            showLEDs();
            delay(600);
            FastLED.clear();
            showLEDs();
            delay(200);
        }
        
        delay(400);
        
        // S (3× kurz)
        for (int i = 0; i < 3; i++) {
            fill_solid(leds, NUM_LEDS, CRGB::Red);
            FastLED.setBrightness(40);
            showLEDs();
            delay(200);
            FastLED.clear();
            showLEDs();
            delay(200);
        }
        
        delay(2000);  // Lange Pause
        yield();
    }
}

bool detectPowerLoss() {
    // ═══ PRÜFE OTA-UPDATE-FLAG ZUERST ═══
    uint8_t otaFlags = EEPROM.read(ADDR_OTA_FLAGS);
    if (otaFlags == OTA_FLAG_UPDATE_SUCCESS) {
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println(  "║   OTA UPDATE ERFOLGREICH               ║");
        Serial.println(  "╚════════════════════════════════════════╝");
        Serial.println("✓ Neustart nach OTA-Update");
        Serial.println("✓ Kein Stromausfall\n");

        // Lösche OTA-Flag (einmalig)
        EEPROM.write(ADDR_OTA_FLAGS, 0);
        EEPROM.commit();

        return false;  // KEIN Stromausfall!
    }

    bool rtcAvailable = rtcPresent && rtc.isrunning();

    if (rtcAvailable)
    {
        Serial.println("→ RTC verfügbar - nutze RTC-Methode");
        return detectPowerLossWithRTC();  // Die vorherige Methode
    } else {
        Serial.println("→ Keine RTC - nutze EEPROM-Methode");

        // Kombiniere Reset-Reason + EEPROM-Flag
        bool badResetReason = detectPowerLossFromResetReason();
        bool runningFlagSet = detectPowerLossWithoutRTC();

        // Wenn BEIDES zutrifft → definitiv Stromausfall
        if (badResetReason && runningFlagSet) {
            Serial.println("\n⚠⚠⚠ STROMAUSFALL BESTÄTIGT");
            Serial.println("  (Reset-Grund + Running-Flag)");
            #ifndef POWERLOSSDETECT
              return false;
            #endif
            #ifdef POWERLOSSDETECT
              return POWERLOSSDETECT;
            #endif
        }

        // Wenn nur Running-Flag → wahrscheinlich Stromausfall
        if (runningFlagSet) {
            Serial.println("\n⚠ Wahrscheinlicher Stromausfall");
            #ifndef POWERLOSSDETECT
              return false;
            #endif
            #ifdef POWERLOSSDETECT
              return POWERLOSSDETECT;
            #endif
        }

        // Wenn nur Reset-Reason aber kein Flag → erste Inbetriebnahme oder Reset-Taste
        if (badResetReason && !runningFlagSet) {
            Serial.println("\n✓ Erste Inbetriebnahme oder Reset-Taste");
            return false;
        }

        return false;
    }
}

// ════════════════════════════════════════════════════════════════
// WIFI STATION SETUP
// ════════════════════════════════════════════════════════════════
static WiFiEventHandler s_disconnectHandler;

static const char* wifiDisconnectReasonName(uint8_t r) {
    switch (r) {
        case REASON_UNSPECIFIED:              return "UNSPECIFIED";
        case REASON_AUTH_EXPIRE:              return "AUTH_EXPIRE";
        case REASON_AUTH_LEAVE:               return "AUTH_LEAVE";
        case REASON_ASSOC_EXPIRE:             return "ASSOC_EXPIRE";
        case REASON_ASSOC_TOOMANY:            return "ASSOC_TOOMANY";
        case REASON_NOT_AUTHED:               return "NOT_AUTHED";
        case REASON_NOT_ASSOCED:              return "NOT_ASSOCED";
        case REASON_ASSOC_LEAVE:              return "ASSOC_LEAVE";
        case REASON_ASSOC_NOT_AUTHED:         return "ASSOC_NOT_AUTHED";
        case REASON_4WAY_HANDSHAKE_TIMEOUT:   return "4WAY_HANDSHAKE_TIMEOUT (falsches Passwort?)";
        case REASON_GROUP_KEY_UPDATE_TIMEOUT: return "GROUP_KEY_UPDATE_TIMEOUT";
        case REASON_IE_IN_4WAY_DIFFERS:       return "IE_IN_4WAY_DIFFERS";
        case REASON_GROUP_CIPHER_INVALID:     return "GROUP_CIPHER_INVALID";
        case REASON_PAIRWISE_CIPHER_INVALID:  return "PAIRWISE_CIPHER_INVALID";
        case REASON_AKMP_INVALID:             return "AKMP_INVALID";
        case REASON_UNSUPP_RSN_IE_VERSION:    return "UNSUPP_RSN_IE_VERSION";
        case REASON_INVALID_RSN_IE_CAP:       return "INVALID_RSN_IE_CAP";
        case REASON_802_1X_AUTH_FAILED:       return "802.1X_AUTH_FAILED (RADIUS lehnt Username/Passwort ab)";
        case REASON_CIPHER_SUITE_REJECTED:    return "CIPHER_SUITE_REJECTED";
        case REASON_BEACON_TIMEOUT:           return "BEACON_TIMEOUT";
        case REASON_NO_AP_FOUND:              return "NO_AP_FOUND";
        case REASON_AUTH_FAIL:                return "AUTH_FAIL";
        case REASON_ASSOC_FAIL:               return "ASSOC_FAIL";
        case REASON_HANDSHAKE_TIMEOUT:        return "HANDSHAKE_TIMEOUT";
        default:                              return "unbekannt";
    }
}

// Konfiguriert die Station (Hostname, IP, Enterprise) und startet den
// Verbindungsaufbau – nicht blockierend. Wird beim Boot und fuer spaetere
// Wiederholungsversuche aus checkWiFiConnection() genutzt.
void beginStation() {

    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN(  "║   WIFI STATION VERBINDUNG              ║");
    DEBUG_PRINTLN(  "╚════════════════════════════════════════╝");
    DEBUG_PRINTF("SSID: %s\n", staSsid);

    // DHCP-Hostname (Option 12) setzen, falls konfiguriert. Manche Cisco-ISE-
    // Policies pruefen den Hostname beim Endpoint-Profiling. Muss vor
    // WiFi.begin/wifi_station_connect aufgerufen werden, damit der erste
    // DHCP-Request den Wert mitsendet.
    if (strlen(staHostname) > 0) {
        DEBUG_PRINTF("Hostname: %s\n", staHostname);
        WiFi.hostname(staHostname);
    }

    // Statische IP konfigurieren (vor WiFi.begin!)
    if (!staDhcp) {
        DEBUG_PRINTLN("Verwende statische IP:");
        DEBUG_PRINTF("  IP:      %s\n", staIP.toString().c_str());
        DEBUG_PRINTF("  Gateway: %s\n", staGateway.toString().c_str());
        DEBUG_PRINTF("  Subnet:  %s\n", staSubnet.toString().c_str());
        DEBUG_PRINTF("  DNS:     %s\n", staDNS.toString().c_str());

        WiFi.config(staIP, staGateway, staSubnet, staDNS);
    } else {
        DEBUG_PRINTLN("Verwende DHCP");
    }

    // Disconnect-Reason als Klartext loggen (hilfreich fuer Enterprise-Debug).
    // Achtung: laeuft im SDK-(sys-)Context – KEIN yield/flush hier, sonst Panic.
    s_disconnectHandler = WiFi.onStationModeDisconnected(
        [](const WiFiEventStationModeDisconnected& evt) {
            Serial.printf("\xE2\x9C\x97 WiFi disconnect: reason=%u (%s)\n",
                          evt.reason, wifiDisconnectReasonName(evt.reason));
        });

    // Auto-Reconnect aktivieren
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);  // Kein Flash-Write bei jedem Connect

    // Modem-Sleep waehrend Connect/EAP deaktivieren: das ESP geht sonst
    // zwischen Beacons in Sleep, wodurch lange EAP-Fragmente (Server-Cert
    // ueber mehrere Frames) gerne mal verschluckt werden – bekannter
    // Workaround fuer sporadische 802.1X-Fails.
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // TX-Power auf Maximum (20.5 dBm). Falls der AP am Limit der Reichweite
    // steht, gehen sonst gerade die laengsten EAP-Frames verloren.
    WiFi.setOutputPower(20.5);

    // Verbindung starten
    if (staEnterprise && strlen(staIdentity) > 0) {
        DEBUG_PRINTLN("→ WPA2-Enterprise (PEAP/MS-CHAPv2)");
        DEBUG_PRINTF("  Username: %s\n", staIdentity);

        // SSID via SDK setzen (kein PSK, das uebernimmt der Enterprise-Layer)
        // _current: NICHT ins Flash schreiben (wifi_station_set_config()
        // loescht bei jedem Aufruf einen Flash-Sektor – bei Wiederholungs-
        // versuchen alle 2 min waere das Flash bald verschlissen). Vorher
        // trennen, damit kein halber Versuch mit alter Konfiguration laeuft.
        wifi_station_disconnect();
        struct station_config conf;
        memset(&conf, 0, sizeof(conf));
        strncpy((char*)conf.ssid, staSsid, sizeof(conf.ssid));
        wifi_station_set_config_current(&conf);

        // Enterprise-Auth aktivieren; ohne CA-Cert wird der RADIUS-Server
        // NICHT verifiziert (bewusste Entscheidung – siehe vars.inc).
        // Bewusst KEINE clear_*-Calls vorab: das Espressif-SDK haelt sonst
        // einen leeren outer identity-String fest, statt auf den eingebauten
        // Default ("anonymous@espressif.com") zurueckzufallen, was manche
        // RADIUS-Server (Cisco ISE) als ungueltig zurueckweisen.
        wifi_station_set_wpa2_enterprise_auth(1);

        // Server-Cert-Validitaetspruefung (NotBefore/NotAfter) abschalten:
        // ohne synchronisierte RTC ist die ESP-Zeit "1970", wodurch jedes
        // Server-Cert als "noch nicht gueltig" gilt und das SDK den TLS-
        // Handshake abbrechen kann – auch wenn wir gar kein CA-Cert gesetzt
        // haben, prueft der SDK-Code intern manchmal trotzdem.
        wifi_station_set_enterprise_disable_time_check(1);

        // CA-Cert setzen, wenn in ca_cert.inc gepflegt. Ohne Cert wird der
        // RADIUS-Server NICHT verifiziert (MITM-anfaellig, deshalb bei
        // Enterprise-Netzen mit "Validate Server Cert" in der Policy
        // wahrscheinlich der Grund fuer reason=23). Bei gesetztem Cert
        // verifiziert das SDK die Cert-Chain – die NotBefore/NotAfter-
        // Pruefung haben wir oben bewusst deaktiviert.
        size_t caLen = strlen(wpa2_ca_cert);
        if (caLen > 0) {
            DEBUG_PRINTF("  CA-Cert:  %u bytes (Server wird verifiziert)\n",
                         (unsigned)caLen);
            // Laenge inkl. abschliessendem '\0' (wie sizeof() in den Espressif-
            // Beispielen) – PEM-Parser erwarten den Terminator mitgezaehlt.
            wifi_station_set_enterprise_ca_cert((uint8_t*)wpa2_ca_cert, caLen + 1);
        } else {
            DEBUG_PRINTLN("  CA-Cert:  <keiner – Server wird NICHT verifiziert>");
        }

        // Outer Identity (anonymous identity), geht im Klartext ueber die Luft
        // und entscheidet beim RADIUS ueber Realm-Routing/Policy:
        // - Feld leer -> Benutzername als Outer Identity. Genau das tun
        //   Android/Windows, wenn "Anonyme Identitaet" leer bleibt (frueher
        //   sendete das ESP hier den SDK-Default "anonymous@espressif.com",
        //   den kein Schul-RADIUS kennt).
        // - "anonymous" / "anonymous@schule.de" -> wie eingetragen.
        const char* outer = strlen(staAnonIdentity) > 0 ? staAnonIdentity : staIdentity;
        DEBUG_PRINTF("  Outer:    %s%s\n", outer, outer == staIdentity ? " (= Benutzername)" : "");
        wifi_station_set_enterprise_identity((uint8_t*)outer, strlen(outer));
        wifi_station_set_enterprise_username((uint8_t*)staIdentity, strlen(staIdentity));
        wifi_station_set_enterprise_password((uint8_t*)staPassword, strlen(staPassword));

        wifi_station_connect();
    } else {
        WiFi.begin(staSsid, staPassword);
    }
}

void setupWiFiStation() {
    if (!staEnabled || strlen(staSsid) == 0) {
        DEBUG_PRINTLN("→ WiFi Station deaktiviert (keine Credentials)");
        return;
    }
    beginStation();

    // Warte max. 10 Sekunden auf Verbindung
    DEBUG_PRINT("Verbinde");
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        DEBUG_PRINT(".");
        timeout++;
    }
    DEBUG_PRINTLN();

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("✓ WiFi verbunden!");
        DEBUG_PRINTF("  IP: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("  Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        DEBUG_PRINTF("  DNS: %s\n", WiFi.dnsIP().toString().c_str());
        DEBUG_PRINTF("  Kanal: %d\n", WiFi.channel());
    } else {
        DEBUG_PRINTLN("⚠ Verbindung fehlgeschlagen (Timeout)");
        DEBUG_PRINTF("  Status: %d\n", WiFi.status());

        // Dauerndes Auto-Reconnect wuerde den AP (gleiches Funkmodul) stoeren,
        // solange jemand ueber den AP konfiguriert. Stattdessen versucht
        // checkWiFiConnection() es alle 2 min erneut, sobald kein Geraet am
        // AP haengt (z.B. Router nach Stromausfall noch nicht bereit).
        WiFi.setAutoReconnect(false);
        WiFi.disconnect();

        // AP-Mode sicherstellen
        WiFi.mode(WIFI_AP_STA);  // Dual-Mode beibehalten
        DEBUG_PRINTLN("→ Neuer Verbindungsversuch in 2 min, AP bleibt vorerst aktiv");
    }
}

// ════════════════════════════════════════════════════════════════
// NTP SETUP & SYNC
// ════════════════════════════════════════════════════════════════
// Wird aus dem SNTP-Stack gerufen, sobald eine neue NTP-Zeit gesetzt wurde
// (standardmaessig einmal pro Stunde). Nur Flag setzen – die Auswertung
// passiert in checkNTPSync() im normalen loop()-Kontext.
static volatile bool sntpTimeReceived = false;
static void onSntpTimeSet() { sntpTimeReceived = true; }

void setupNTP()
{
    if (!ntpEnabled || WiFi.status() != WL_CONNECTED)
    {
        DEBUG_PRINTLN("\n╔══════════════════════════════════════════════════╗");
        DEBUG_PRINTLN(  "║ → NTP nicht möglich (deaktiviert oder kein WiFi) ║");
        DEBUG_PRINTLN(  "╚══════════════════════════════════════════════════╝");
        return;
    }

    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN(  "║   NTP ZEITSERVER-SYNC                  ║");
    DEBUG_PRINTLN(  "╚════════════════════════════════════════╝");
    DEBUG_PRINTF("NTP Server: %s\n", ntpServer);

    static bool callbackRegistered = false;
    if (!callbackRegistered) {
        settimeofday_cb(onSntpTimeSet);
        callbackRegistered = true;
    }

    // Konfiguriere NTP (UTC speichern, Zeitzone wird beim Anzeigen umgerechnet).
    // Nicht blockierend: die Zeit wird uebernommen, sobald das erste Paket
    // eintrifft (Callback -> checkNTPSync()).
    configTime(0, 0, ntpServer, "pool.ntp.org", "time.nist.gov");
    DEBUG_PRINTLN("ℹ NTP gestartet – Zeit wird mit dem ersten Paket uebernommen");
}

// Uebernimmt eine frisch per SNTP gesetzte Systemzeit: ESP-Drift messen,
// Zeitanker setzen, RTC vermessen/nachstellen, alles mit EINEM
// EEPROM.commit() speichern (jedes commit loescht einen Flash-Sektor).
void onNtpTimeReceived() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    if (tv.tv_sec < 1735689600) return;  // noch keine gueltige Zeit
    int64_t realMs = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    measureEspDrift(realMs, 3000);
    anchorTime(tv.tv_sec, true, realMs);
    lastNtpSync = tv.tv_sec;
    lastSyncTime = tv.tv_sec;
    ntpSyncSuccessful = true;

    updateRTCDriftCalibration();  // blockiert ggf. bis ~3 s

    saveLastKnownTime(false);
    saveNTPConfig(false);
    saveDriftRate(false);
    saveRTCDrift(false);
    EEPROM.commit();
    DEBUG_PRINTF("✓ NTP-Zeit uebernommen: %lu\n", (unsigned long)tv.tv_sec);
}

void checkNTPSync() {
    if (sntpTimeReceived) {
        sntpTimeReceived = false;
        if (ntpEnabled) onNtpTimeReceived();
    }
    // Laenger als 2 h keine NTP-Zeit mehr -> nicht mehr als synchron werten
    if (ntpSyncSuccessful && lastNtpSync > 0) {
        unsigned long now = nowUtc();
        if (now > lastNtpSync && now - lastNtpSync > 7200) {
            ntpSyncSuccessful = false;
            DEBUG_PRINTLN("⚠ Seit 2 h keine NTP-Zeit erhalten");
        }
    }
}

// ════════════════════════════════════════════════════════════════
// WIFI VERBINDUNGS-MONITOR
// ════════════════════════════════════════════════════════════════
unsigned long lastWiFiCheck = 0;
bool wasConnected = false;

void checkWiFiConnection() {
    // Nur alle 10 Sekunden prüfen
    if (millis() - lastWiFiCheck < 10000) {
        return;
    }
    lastWiFiCheck = millis();

    // Nur wenn Station aktiviert ist
    if (!staEnabled || strlen(staSsid) == 0) {
        return;
    }

    bool isConnected = (WiFi.status() == WL_CONNECTED);

    // Verbindung verloren?
    if (wasConnected && !isConnected) {
        DEBUG_PRINTLN("⚠ WiFi-Verbindung verloren!");
        DEBUG_PRINTF("  Status: %d\n", WiFi.status());

        // Auto-Reconnect ist aktiv, ESP verbindet automatisch neu
        DEBUG_PRINTLN("  → Auto-Reconnect aktiv...");
    }

    // Verbindung wiederhergestellt?
    if (!wasConnected && isConnected)
    {
        DEBUG_PRINTLN("✓ WiFi-Verbindung wiederhergestellt!");
        DEBUG_PRINTF("  IP: %s\n", WiFi.localIP().toString().c_str());

        // NTP neu konfigurieren
        if (ntpEnabled) {
            setupNTP();
        }
    }

    // Nicht verbunden: regelmaessig neu versuchen – aber nicht, solange ein
    // Geraet am AP haengt (der Verbindungsversuch stoert das gemeinsame
    // Funkmodul und damit die laufende Konfiguration).
    static unsigned long lastStationRetry = 0;
    if (!isConnected) {
        bool apInUse = apActive && WiFi.softAPgetStationNum() > 0;
        if (!apInUse && millis() - lastStationRetry >= 120000UL) {
            lastStationRetry = millis();
            DEBUG_PRINTLN("→ Neuer WLAN-Verbindungsversuch...");
            beginStation();
            // Solange der AP laeuft: nur dieser eine Versuch, kein Dauer-
            // Reconnect im Hintergrund, der den AP fuer Clients unbrauchbar macht.
            if (apActive) WiFi.setAutoReconnect(false);
        }
    }
    wasConnected = isConnected;
}

// ════════════════════════════════════════════════════════════════
// AP-TIMEOUT
// ════════════════════════════════════════════════════════════════
// Der Access Point schaltet sich AP_TIMEOUT nach dem Start ab – unabhaengig
// davon, ob ein Heim-WLAN konfiguriert oder verbunden ist. Solange ein
// Geraet mit dem AP verbunden ist, laeuft der Timer nicht ab (sonst fliegt
// man mitten in der Konfiguration raus); er startet neu, sobald es sich
// trennt. Im Stromausfall-Modus bleibt der AP an, damit die Warnung
// quittiert werden kann. Zurueck bekommt man den AP durch einen Neustart
// (Strom aus/an). Die Weboberflaeche bleibt im Heim-WLAN erreichbar.
void checkApTimeout() {
#if defined(DEBUG_MODE) && (DEBUG_MODE == false)
    if (!apActive) return;
    if (powerLossDetected || WiFi.softAPgetStationNum() > 0) {
        apStartTime = millis();
        return;
    }
    if (millis() - apStartTime < AP_TIMEOUT) return;

    dnsServer.stop();
    WiFi.softAPdisconnect(true);  // schaltet nur das AP-Interface ab
    if (!staEnabled || strlen(staSsid) == 0) {
        WiFi.mode(WIFI_OFF);
    } else {
        // Station weiterlaufen lassen und ab jetzt selbststaendig verbinden
        WiFi.setAutoReconnect(true);
    }
    apActive = false;
    DEBUG_PRINTLN("⚠ AP nach Timeout deaktiviert (Neustart aktiviert ihn wieder)");
#else
    #warning "DEBUG_MODE ist nicht false - WiFi AP bleibt immer aktiv"
#endif
}

// ════════════════════════════════════════════════════════════════
// SETUP & LOOP
// ════════════════════════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    while (!Serial) { }
    Serial.setDebugOutput(true);

    // Internen os_printf-Stream des Espressif-NONOS-SDK auf UART0 freischalten.
    // Standardmaessig stummgeschaltet; mit diesem Aufruf werden u.a. EAP-,
    // TLS- und PHY-Logs sichtbar, was zur Diagnose von WPA2-Enterprise-
    // Disconnects (reason=23) hilfreich ist.
    system_set_os_print(1);
    delay(5000);
    Serial.setDebugOutput(false);

    Serial.printf("Flash Chip ID: %08X\n", ESP.getFlashChipId());
    Serial.printf("Flash Chip real size: %u bytes\n", ESP.getFlashChipRealSize());
    Serial.printf("Flash Chip mode: %d\n", ESP.getFlashChipMode());
    Serial.printf("Flash Chip speed: %u Hz\n", ESP.getFlashChipSpeed());
    Serial.printf("SDK-Version: %s\n", ESP.getSdkVersion());
    Serial.printf("Sketch size: %u bytes (Max: %u bytes)\n", ESP.getSketchSize(), ESP.getFreeSketchSpace() + ESP.getSketchSize());
    Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());
    Serial.printf("Flash layout: %s\n", (ESP.getFreeSketchSpace() + ESP.getSketchSize()) > 1100000 ? "4m2m (2MB)" : "4m1m (1MB)");

    DEBUG_PRINTLN("\n╔════════════════════════════════╗");
    DEBUG_PRINTLN(  "║        CharGraph BOOT          ║");
    DEBUG_PRINTLN(  "╚════════════════════════════════╝\n");
    
    // Verdrahtungshinweis anzeigen
    showConnect();
    
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    //Clear leds
    //memset(leds, 0, (size_t) sizeof(leds));
    FastLED.clear(true); 
    FastLED.setBrightness(0);
    // WICHTIG: Setze Correction für ESP8266 mit WiFi
    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setMaxRefreshRate(60);
    showLEDs();
    delay(100);
    // RGB Test ausführen
    DEBUG_PRINT("\n╔════════════════════════════════╗\n");
    DEBUG_PRINT(  "║   Führe RGB-Test aus...        ║\n");
    DEBUG_PRINT(  "╚════════════════════════════════╝\n");
    FastLED.setBrightness(80);
    if(TEST_RGB) rgbTest();
    FastLED.setBrightness(0);
    while(TEST_RGB_ONLY);

    Wire.begin(I2C_SDA, I2C_SCL);

    // RTC nur einmal initialisieren (rtc.begin() legt bei jedem Aufruf ein
    // neues I2C-Geraet auf dem Heap an) – danach nur noch rtcPresent pruefen.
    bool rtcWasStopped = false;
    #if (defined(USE_RTC) && USE_RTC == false)
        #warning "RTC not in use!"
        rtcPresent = false;
    #else
        rtcPresent = rtc.begin();
    #endif
    if (rtcPresent)
    {
        if (!rtc.isrunning()) {
            // Kompilierzeit ist Ortszeit, die RTC laeuft in UTC -> umrechnen
            uint32_t t = DateTime(F(__DATE__), F(__TIME__)).unixtime() - 3600;
            if (isDST(t)) t -= 3600;
            rtc.adjust(DateTime(t));
            rtcWasStopped = true;
        }
        DEBUG_PRINTLN("✓ RTC initialisiert");
    }
    else
    {
      DEBUG_PRINTLN("❌ Keine RTC erkannt");
    }

    EEPROM.begin(EEPROM_SIZE);

    // Zuerst Default charsoap initialisieren
    initCharsoap(charsoap);

    loadCharsoap();
    loadSpecialWords();
    loadMinuteLeds();
    loadSpecialWordInterval();
    loadSpecialWordMode();

    DEBUG_PRINT("\n╔════════════════════════════════╗\n");
    DEBUG_PRINT(  "║   Prüfe Wörter in Liste...     ║\n");
    DEBUG_PRINT(  "╚════════════════════════════════╝\n")
    FastLED.setBrightness(80);
    checkPattern();

    loadConfig();
    loadDriftRate();
    loadRTCDrift();
    if (rtcWasStopped) {
        // RTC wurde eben notduerftig gestellt – alter Referenzpunkt ungueltig
        rtcRefTime = 0;
        rtcRefOffset = 0.0f;
        rtcRefPrecise = false;
    }
    loadOTAVersion();
    loadWiFiStationConfig();
    loadNTPConfig();
    loadAutoBrightnessConfig();

    // ═══ STROMAUSFALL-PRÜFUNG ═══
    // Bei erkanntem Stromausfall NICHT mehr blockierend in powerLossLoop()
    // bleiben – stattdessen Flag setzen, WLAN + Webserver normal starten,
    // im loop() wird SOS gerendert bis der User in der UI quittiert.
    powerLossDetected = detectPowerLoss();
    if (powerLossDetected) {
        Serial.println("⚠ Stromausfall – Webserver wird gestartet, SOS laeuft bis Quittierung");
    } else {
        // Nur im Normalfall direkt das Running-Flag setzen – sonst wuerde
        // ein Reset waehrend des Stromausfall-Modus den Marker auf 'lief'
        // setzen, ohne dass der User die Zeit jemals quittiert hat.
        setRunningFlag();
        // Frischen "letzte bekannte Zeit"-Wert ablegen, sonst koennte ein
        // schneller Stromaus kurz nach dem Boot den naechsten Check noch
        // gegen den 8 Tage alten Wert laufen lassen.
        saveLastKnownTime();
    }
    // Nach einem OTA-Neustart wird die RTC in detectPowerLoss() nicht gelesen –
    // dann hier nachholen, sonst liefe die Uhr mit der zuletzt gespeicherten
    // Zeit weiter (bis zu 1 h alt).
    if (!anchorValid && rtcPresent) {
        unsigned long t = getCorrectedRTCTime();
        if (t > 1735689600UL) anchorTime(t, false);
    }

    // User-Helligkeit (0-80) auf LED-Helligkeit (0-204) mappen
    FastLED.setBrightness(map(brightness, 0, 80, 0, 204));

    // Keine WLAN-Einstellungen bei jedem Boot/Verbindungsaufbau ins Flash schreiben
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);  // Dual-Mode: AP + Station
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    DEBUG_PRINTF("SSID: %s\n", AP_SSID);
    DEBUG_PRINTF("IP: %s\n\n", WiFi.softAPIP().toString().c_str());

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    // WiFi Station verbinden (wenn konfiguriert)
    setupWiFiStation();

    // NTP konfigurieren (wenn WiFi verbunden)
    if (WiFi.status() == WL_CONNECTED) {
        setupNTP();
    }
    
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.on("/gettime", handleGetTime);
    server.on("/colors/get", handleGetColors);
    server.on("/powerloss/clear", handlePowerLossClear);
    server.on("/rtcdrift/lock", handleRTCDriftLock);
    server.on("/rtcdrift/reset", handleRTCDriftReset);
    server.on("/getcharsoap", handleGetCharsoap);
    server.on("/resetcharsoap", handleResetCharsoap);
    server.on("/ledtest", handleLEDTest);
    server.on("/patterntest", handlePatternTest);
    server.on("/ota/info", handleOTAInfo);
    server.on("/ota/upload", HTTP_POST, handleOTAUploadDone, handleOTAUpload);
    server.on("/ota/url", handleOTAFromURL);
    server.on("/ota/status", handleOTAStatus);
    server.on("/wifi/config", handleGetWiFiConfig);
    server.on("/wifi/save", handleSaveWiFiConfig);
    server.on("/wifi/scan", handleWiFiScan);
    server.on("/restart", handleRestart);
    server.on("/autobrightness/config", handleGetAutoBrightness);
    server.on("/autobrightness/save", handleSaveAutoBrightness);
    server.on("/specialwords/get", handleGetSpecialWords);
    server.on("/specialwords/save", handleSaveSpecialWords);
    server.on("/specialwords/reset", handleResetSpecialWords);
    server.on("/minuteleds/get", handleGetMinuteLeds);
    server.on("/minuteleds/save", handleSaveMinuteLeds);
    server.on("/minuteleds/reset", handleResetMinuteLeds);
    server.onNotFound(handleNotFound);
    server.begin();
    
    apActive    = true;
    apStartTime = millis();
    
    DEBUG_PRINTLN("✓ System bereit!\n");
    // Initial-Frame schwarz zeigen, aber die zuvor gesetzte User-Helligkeit
    // beibehalten – sonst bleibt der Strip nach dem Boot dunkel (displayTime
    // setzt die Brightness bewusst nicht mehr, damit Auto-Brightness nicht
    // ueberschrieben wird). Frueher stand hier setBrightness(0) und funk-
    // tionierte nur, weil displayTime damals zusaetzlich setBrightness(80)
    // rief.
    FastLED.clear(true);
    yield();
    delay(1000);
}

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS MIT ADC (A0)
// ════════════════════════════════════════════════════════════════
unsigned long lastBrightnessUpdate = 0;
const unsigned long brightnessUpdateInterval = 500;  // 500 ms = 2 Hz, schnell aber nicht flackernd

void updateBrightness() {
       yield();
    // Nur alle 2 Sekunden aktualisieren (verhindert Flackern und spart CPU)
    if (millis() - lastBrightnessUpdate < brightnessUpdateInterval) {
        yield();
        return;
    }
    
    if (!autoBrightnessEnabled)
    {
        return;  // Funktion deaktiviert
    }
    
    lastBrightnessUpdate = millis();

    // ADC mehrfach auslesen und Mittelwert bilden (reduziert Rauschen)
    uint32_t adcSum = 0;
    const uint8_t samples = 10;
    for (uint8_t i = 0; i < samples; i++) {
        adcSum += analogRead(A0);
        delay(1);  // Kurze Pause zwischen Messungen
    }
    uint16_t adcValue = adcSum / samples;

    // Exponentieller Tiefpass: 75% alter Wert + 25% neuer Wert.
    // Glaettet schnelle Schwankungen (z.B. Schatten, Mauszeiger ueber Sensor)
    // ohne die Reaktion auf echte Helligkeitsaenderungen merklich zu bremsen.
    static uint16_t smoothedAdc = 0;
    if (smoothedAdc == 0) smoothedAdc = adcValue;  // Erstinitialisierung
    smoothedAdc = (smoothedAdc * 3 + adcValue) / 4;
    adcValue = smoothedAdc;

    // Ungueltige Kalibrierung (Min >= Max) -> map() wuerde durch 0 teilen
    if (autoBrightnessMaxADC <= autoBrightnessMinADC) return;

    // Auf kalibrierten Bereich begrenzen
    if (adcValue < autoBrightnessMinADC) adcValue = autoBrightnessMinADC;
    if (adcValue > autoBrightnessMaxADC) adcValue = autoBrightnessMaxADC;

    // Linear auf autoBrightnessMin-autoBrightnessMax mappen
    uint8_t newBrightness = map(adcValue,
                                 autoBrightnessMinADC,
                                 autoBrightnessMaxADC,
                                 autoBrightnessMin,
                                 autoBrightnessMax);

    // User-Helligkeit (0-80) auf LED-Helligkeit (0-204) mappen
    // 80 = 100% User-Helligkeit entspricht 204 = 80% LED-Helligkeit
    newBrightness = map(newBrightness, 0, 80, 0, 204);

    // Helligkeit setzen + sofort an die LEDs senden (sonst wirkt der neue
    // Wert erst beim naechsten Minuten-Refresh durch displayTime). Nur bei
    // Aenderung – sonst wird zweimal pro Sekunde unnoetig ein Frame geschickt.
    if (newBrightness != FastLED.getBrightness()) {
        FastLED.setBrightness(newBrightness);
        showLEDs();
    }
}

void loop()
{
    // Webserver, WLAN-Ueberwachung und NTP laufen IMMER – nicht nur solange
    // der AP aktiv ist. Frueher hing alles an apActive: nach einem AP-Timeout
    // waere die Uhr auch im Heim-WLAN nicht mehr erreichbar gewesen und haette
    // keine NTP-Zeit mehr uebernommen.
    if (apActive) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
    checkWiFiConnection();   // Station ueberwachen, ggf. neu verbinden
    checkNTPSync();          // neue SNTP-Zeit uebernehmen
    checkApTimeout();        // AP nach Timeout abschalten
    handlePendingRestart();  // z.B. nach OTA, nachdem die Antwort raus ist

    // Aktuelle Uhrzeit stuendlich ins EEPROM schreiben, damit
    // detectPowerLossWithRTC beim naechsten Boot den echten Offline-Zeitraum
    // kennt. Nicht im Stromausfall-Modus: sonst wuerde ein Neustart die noch
    // nicht quittierte Warnung stillschweigend verschwinden lassen.
    if (!powerLossDetected && millis() - lastTimePersistMs >= 3600000UL) {
        saveLastKnownTime();
    }

  // Stromausfall-Modus: nur SOS rendern, normale Anzeige uebergehen.
  // Webserver und Auto-Reconnect laufen oben weiter, der User kann die
  // Zeit setzen und ueber /powerloss/clear quittieren.
  if (powerLossDetected) {
    renderPowerLossSOS();
    yield();
    return;
  }

  // Auto-Brightness (intern auf brightnessUpdateInterval gedrosselt) – erst
  // nach dem SOS-Zweig, sonst ueberschreibt sie dessen feste Helligkeit.
  updateBrightness();

  // Laufender Pattern-Test: normale Zeitanzeige aussetzen
  if (patternTestRunning) {
    runPatternTestStep();
    yield();
    return;
  }

  // Zeit seit der letzten Anzeige prüfen
  if (millis() - lastUpdateTime >= updateInterval)
  {
    int hours, minutes, seconds;
    getCurrentTime(hours, minutes, seconds);
    if (minutes != lastDisplayedMinute)
    {
      switch (specialWordMode) {
        case SPECIAL_WORD_MODE_PARALLEL:
          displayTimeWithSpecial(hours, minutes);
          break;
        case SPECIAL_WORD_MODE_INTERVAL:
          if (shouldShowSpecialWord(minutes)) showSpecialWordThenTime(hours, minutes);
          else                                displayTime(hours, minutes);
          break;
        case SPECIAL_WORD_MODE_OFF:
        default:
          displayTime(hours, minutes);
          break;
      }
      lastDisplayedMinute = minutes;
    }
    lastUpdateTime = millis();
  }
  yield();
}
