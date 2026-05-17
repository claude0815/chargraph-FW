#include <common.inc>
#include <Updater.h>
#include <ca_cert.inc>

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

    // Standard-Werte aus rgbPanel.cpp
    const char DEFAULT_SPECIAL_WORD[MAXWORDS][12] = {
        "RWD",
        "\0",
        "\0"
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
    EEPROM.write(ADDR_CONFIGURED, MAGIC_BYTE_INIT);
    
    EEPROM.write(ADDR_TIMESTAMP, (bootTime >> 24) & 0xFF);
    EEPROM.write(ADDR_TIMESTAMP + 1, (bootTime >> 16) & 0xFF);
    EEPROM.write(ADDR_TIMESTAMP + 2, (bootTime >> 8) & 0xFF);
    EEPROM.write(ADDR_TIMESTAMP + 3, bootTime & 0xFF);
    
    EEPROM.commit();
}

void loadDriftRate() {
    lastSyncTime = 0;
    lastSyncTime |= ((unsigned long)EEPROM.read(ADDR_LAST_SYNC)) << 24;
    lastSyncTime |= ((unsigned long)EEPROM.read(ADDR_LAST_SYNC + 1)) << 16;
    lastSyncTime |= ((unsigned long)EEPROM.read(ADDR_LAST_SYNC + 2)) << 8;
    lastSyncTime |= EEPROM.read(ADDR_LAST_SYNC + 3);
    
    byte driftBytes[4];
    for (int i = 0; i < 4; i++) {
        driftBytes[i] = EEPROM.read(ADDR_DRIFT_RATE + i);
    }
    driftRate = *((float*)driftBytes);
    
    syncCount = 0;
    syncCount |= EEPROM.read(ADDR_SYNC_COUNT) << 8;
    syncCount |= EEPROM.read(ADDR_SYNC_COUNT + 1);
    
    if (lastSyncTime < 1735689600) lastSyncTime = 0;
    if (isnan(driftRate) || driftRate < -10.0 || driftRate > 10.0) driftRate = 0.0;
    if (syncCount < 0 || syncCount > 1000) syncCount = 0;
}

void saveDriftRate() {
    EEPROM.write(ADDR_LAST_SYNC, (lastSyncTime >> 24) & 0xFF);
    EEPROM.write(ADDR_LAST_SYNC + 1, (lastSyncTime >> 16) & 0xFF);
    EEPROM.write(ADDR_LAST_SYNC + 2, (lastSyncTime >> 8) & 0xFF);
    EEPROM.write(ADDR_LAST_SYNC + 3, lastSyncTime & 0xFF);
    
    byte* driftBytes = (byte*)&driftRate;
    for (int i = 0; i < 4; i++) {
        EEPROM.write(ADDR_DRIFT_RATE + i, driftBytes[i]);
    }
    
    EEPROM.write(ADDR_SYNC_COUNT, (syncCount >> 8) & 0xFF);
    EEPROM.write(ADDR_SYNC_COUNT + 1, syncCount & 0xFF);
    
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

    const char* buildDate = BUILD_DATE;
    const char* buildTime = BUILD_TIME;
    int idx = 0;
    for (uint8_t i = 0; i < strlen(buildDate) && idx < 11; i++, idx++) {
        EEPROM.write(ADDR_OTA_BUILD_DATE + idx, buildDate[i]);
    }
    EEPROM.write(ADDR_OTA_BUILD_DATE + idx++, ' ');
    for (uint8_t i = 0; i < strlen(buildTime) && idx < 20; i++, idx++) {
        EEPROM.write(ADDR_OTA_BUILD_DATE + idx, buildTime[i]);
    }
    EEPROM.write(ADDR_OTA_BUILD_DATE + idx, '\0');

    EEPROM.commit();
    DEBUG_PRINTLN("✓ OTA Version gespeichert: " + String(firmwareVersion));
}

void loadOTAVersion() {
    for (int i = 0; i < 32; i++) {
        firmwareVersion[i] = EEPROM.read(ADDR_OTA_VERSION + i);
        if (firmwareVersion[i] == '\0' || firmwareVersion[i] == 0xFF) {
            firmwareVersion[i] = '\0';
            break;
        }
    }

    if (firmwareVersion[0] == '\0' || firmwareVersion[0] == 0xFF) {
        generateVersion(firmwareVersion, sizeof(firmwareVersion));
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

    // Outer/Anonymous Identity laden; Fallback Default = "anonymous"
    bool anonFound = false;
    for (int i = 0; i < 64; i++) {
        byte c = EEPROM.read(ADDR_STA_ENTERPRISE_ANON_ID + i);
        if (c == 0 || c == 0xFF) {
            staAnonIdentity[i] = '\0';
            if (i > 0) anonFound = true;
            break;
        }
        staAnonIdentity[i] = c;
        anonFound = true;
    }
    staAnonIdentity[63] = '\0';
    if (!anonFound) {
        strncpy(staAnonIdentity, "anonymous", sizeof(staAnonIdentity) - 1);
        staAnonIdentity[sizeof(staAnonIdentity) - 1] = '\0';
    }

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

void saveNTPConfig() {
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

    EEPROM.commit();
    DEBUG_PRINTLN("✓ NTP Config gespeichert");
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

    // Validierung
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
  FastLED.setBrightness(80);
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
      
      // Hervorhebung für Stundenwort (letztes vor UHR)
      if (i == (result.wordCount - 1 - hasUhr))
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
void getCurrentTime(int &hours, int &minutes, int &seconds) {
    static unsigned long lastMillis = 0;
    static unsigned long millisOverflows = 0;
    static unsigned long lastRTCSync = 0;
    
    unsigned long currentMillis = millis();
    
    if (currentMillis < lastMillis) {
        millisOverflows++;
        DEBUG_PRINTLN("⚠ millis() Overflow!");
    }
    lastMillis = currentMillis;
    
    unsigned long totalSeconds = (millisOverflows * 4294967UL) + (currentMillis / 1000);
    unsigned long currentSeconds = bootTime + totalSeconds;
    
    if (currentMillis - lastRTCSync > 86400000 || lastRTCSync > currentMillis) {
        if (rtc.begin() && rtc.isrunning()) {
            DateTime rtcNow = rtc.now();
            unsigned long rtcTime = rtcNow.unixtime();
            long drift = currentSeconds - rtcTime;
            
            if (abs(drift) > 30) {
                bootTime = rtcTime - totalSeconds;
                currentSeconds = rtcTime;
                DEBUG_PRINTF("RTC-Sync: %ld Sek\n", drift);
            }
        }
        lastRTCSync = currentMillis;
    }
    
    if (driftRate != 0.0 && lastSyncTime > 0) {
        float daysSinceSync = (currentSeconds - lastSyncTime) / 86400.0;
        long estimatedDrift = (long)(driftRate * daysSinceSync);
        currentSeconds -= estimatedDrift;
    }

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

// ════════════════════════════════════════════════════════════════
// WEBSERVER HANDLER
// ════════════════════════════════════════════════════════════════
void handleRoot() {
    DEBUG_PRINTLN("→ handleRoot aufgerufen");

    // GZIP-komprimierte HTML-Seite senden
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html", (const char*)HTML_PAGE_GZIP, HTML_PAGE_GZIP_LEN);

    DEBUG_PRINTF("  HTML gesendet: %d Bytes (komprimiert)\n", HTML_PAGE_GZIP_LEN);
}

void handleGetColors() {
    String json = "{";
    json += "\"nr\":" + String(normalColor.r) + ",";
    json += "\"ng\":" + String(normalColor.g) + ",";
    json += "\"nb\":" + String(normalColor.b) + ",";
    json += "\"sr\":" + String(specialColor.r) + ",";
    json += "\"sg\":" + String(specialColor.g) + ",";
    json += "\"sb\":" + String(specialColor.b) + ",";
    json += "\"brightness\":" + String(brightness) + ",";
    json += "\"rainbow\":" + String(useRainbow ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
}

void handleGetTime() {
    DEBUG_PRINTLN("→ handleGetTime aufgerufen");
    int h, m, s;
    getCurrentTime(h, m, s);

    // UTC-Zeit berechnen
    unsigned long currentSecondsUTC = bootTime + (millis() / 1000);

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
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"powerLoss\":" + String(powerLossDetected ? "true" : "false");
    json += "}";

    server.send(200, "application/json", json);
}

void handlePowerLossClear() {
    DEBUG_PRINTLN("→ handlePowerLossClear aufgerufen");
    clearPowerLossWarning();
    server.send(200, "text/plain", "OK - Stromausfall-Warnung quittiert");
}

// ════════════════════════════════════════════════════════════════
// PATTERN TEST HANDLER
// ════════════════════════════════════════════════════════════════
void handlePatternTest() {
    DEBUG_PRINTLN("→ handlePatternTest aufgerufen");

    if (patternTestRunning) {
        server.send(409, "text/plain", "Pattern-Test läuft bereits!");
        return;
    }

    server.send(200, "text/plain", "Pattern-Test gestartet - Nur kritische Zeiten werden getestet");

    patternTestRunning = true;

    // Kritische Test-Zeiten: Nur Zeiten mit Wortüberlappungen
    const int testTimes[][2] = {
        // Stunde 0 und 1 KOMPLETT (EIN vs EINS, mit/ohne UHR)
        {0, 0}, {0, 5}, {0, 10}, {0, 15}, {0, 20}, {0, 25}, {0, 30}, {0, 35}, {0, 40}, {0, 45}, {0, 50}, {0, 55},
        {1, 0}, {1, 5}, {1, 10}, {1, 15}, {1, 20}, {1, 25}, {1, 30}, {1, 35}, {1, 40}, {1, 45}, {1, 50}, {1, 55},

        // Beispiel weitere Stunden
        {12, 0}, {12, 25},  // ZWÖLF und HALB EINS

        // VIER als Stunde (weil "VIER" zweimal vorkommt: in VIERTEL + separat)
        {3, 45},  // VIERTEL VOR VIER
        {4, 0},   // VIER UHR
        {4, 5}, {4, 10}, {4, 15}, {4, 20}, {4, 25}, {4, 30}, {4, 35}, {4, 40}, {4, 45}, {4, 50}, {4, 55},

        // FÜNF als Stunde (weil "FuNF" zweimal vorkommt: separat + in ZWoLFuNF)
        {4, 55},  // FÜNF VOR FÜNF
        {5, 0},   // FÜNF UHR
        {5, 5}, {5, 10}, {5, 15}, {5, 20}, {5, 25}, {5, 30}, {5, 35}, {5, 40}, {5, 45}, {5, 50}, {5, 55},

        // ZEHN als Stunde (weil "ZEHN" zweimal vorkommt: oben + unten)
        {9, 50},  // ZEHN VOR ZEHN
        {10, 0},  // ZEHN UHR
        {10, 5}, {10, 10}, {10, 15}, {10, 20}, {10, 25}, {10, 30}, {10, 35}, {10, 40}, {10, 45}, {10, 50}, {10, 55}
    };

    const int numTests = sizeof(testTimes) / sizeof(testTimes[0]);

    for (int i = 0; i < numTests && patternTestRunning; i++) {
        int h = testTimes[i][0];
        int m = testTimes[i][1];

        DEBUG_PRINTF("Pattern-Test: %02d:%02d (%d/%d)\n", h, m, i+1, numTests);
        displayTime(h, m);

        // 2 Sekunden warten, dabei WiFi am Leben halten
        for (int j = 0; j < 20 && patternTestRunning; j++) {
            yield();
            server.handleClient();
            delay(100);
        }
    }

    patternTestRunning = false;
    DEBUG_PRINTLN("✓ Pattern-Test abgeschlossen");

    // Zurück zur aktuellen Zeit
    int h, m, s;
    getCurrentTime(h, m, s);
    displayTime(h, m);
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
        json += SPECIAL_WORD[i];
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
        unsigned long utcTimestamp = server.arg("timestamp").toInt();
        int timezoneOffset = server.hasArg("tzoffset") ? server.arg("tzoffset").toInt() : 0;
        unsigned long clientTime = utcTimestamp - timezoneOffset;

        // timestamp=0 bedeutet "Zeit nicht aendern" – wird vom Frontend bei
        // saveColors() / saveCharsoap() gesendet, um Drift-Berechnung, bootTime
        // und RTC nicht zu zerschiessen.
        if (utcTimestamp != 0) {
            if (lastSyncTime > 0 && bootTime > 0) {
                unsigned long espTime = bootTime + (millis() / 1000);
                unsigned long timeSinceSync = clientTime - lastSyncTime;

                if (timeSinceSync > 3600) {
                    long drift = espTime - clientTime;
                    float daysElapsed = timeSinceSync / 86400.0;
                    float newDriftRate = drift / daysElapsed;

                    if (syncCount > 0) {
                        driftRate = (driftRate * syncCount + newDriftRate) / (syncCount + 1);
                    } else {
                        driftRate = newDriftRate;
                    }

                    syncCount++;
                    saveDriftRate();
                }
            }

            bootTime = clientTime - (millis() / 1000);
            lastSyncTime = clientTime;

            if (rtc.begin()) {
                DateTime newTime(clientTime);
                rtc.adjust(newTime);
            }
        }
        
        normalColor.r = server.arg("nr").toInt();
        normalColor.g = server.arg("ng").toInt();
        normalColor.b = server.arg("nb").toInt();
        if (server.hasArg("rainbow")) {
            useRainbow = (server.arg("rainbow") == "1" || server.arg("rainbow") == "true");
        }
        
        specialColor.r = server.arg("sr").toInt();
        specialColor.g = server.arg("sg").toInt();
        specialColor.b = server.arg("sb").toInt();
        
        brightness = server.arg("brightness").toInt();

        if (server.hasArg("charsoap")) {
            String newCharsoap = server.arg("charsoap");
            // Achtung: nicht .toUpperCase() – Umlaute werden vom Frontend bereits
            // in Kleinbuchstaben (a/o/u) umgewandelt; ein zweites Uppercasen wuerde
            // diese Information verlieren.
            if (newCharsoap.length() == CHARSOAP_LEN) {
                saveCharsoap(newCharsoap.c_str());
            } else if (newCharsoap.length() > 0) {
                DEBUG_PRINTF("❌ handleSave: charsoap Laenge %u (erwartet %u)\n",
                             newCharsoap.length(), (unsigned)CHARSOAP_LEN);
            }
        }

        saveConfig();
        // User-Helligkeit (0-80) auf LED-Helligkeit (0-204) mappen
        DEBUG_PRINTF("Brightness: %d",(int) map(brightness, 0, 80, 0, 204));
        FastLED.setBrightness(map(brightness, 0, 80, 0, 204));
        
        server.send(200, "text/plain", "OK");
        int hours, minutes, seconds;
        getCurrentTime(hours, minutes, seconds);

        displayTime(hours, minutes);
        lastUpdateTime = millis();
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
    uint8_t testBrightness = server.hasArg("brightness") ? server.arg("brightness").toInt() : 80;
    
    CRGB color = CRGB(r, g, b);
    
    // Aktuelle Helligkeit sichern und Test-Helligkeit setzen
    uint8_t savedBrightness = brightness;
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
    
    // Helligkeit zurücksetzen
    FastLED.setBrightness(savedBrightness);
}

// ════════════════════════════════════════════════════════════════
// OTA UPDATE HANDLER
// ════════════════════════════════════════════════════════════════
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
        if (Update.end(true)) {
            DEBUG_PRINTF("OTA Success: %u bytes\n", upload.totalSize);
            showOTAProgress(100, false, true);

            EEPROM.write(ADDR_OTA_FLAGS, OTA_FLAG_UPDATE_SUCCESS);
            EEPROM.commit();

            otaInProgress = false;

            delay(3000);
            ESP.restart();
        } else {
            Update.printError(Serial);
            otaError = String(Update.getError());
            showOTAProgress(100, true, false);
            otaInProgress = false;
        }
    }
}

void handleOTAUploadDone() {
    if (otaError.length() > 0) {
        String json = "{\"success\":false,\"message\":\"" + otaError + "\"}";
        server.send(500, "application/json", json);
    } else {
        String json = "{\"success\":true,\"message\":\"Update erfolgreich\"}";
        server.send(200, "application/json", json);
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
            EEPROM.commit();

            delay(3000);
            ESP.restart();
            break;
    }
}

void handleOTAStatus() {
    DEBUG_PRINTLN("→ handleOTAStatus aufgerufen");

    String json = "{";
    json += "\"inProgress\":" + String(otaInProgress ? "true" : "false") + ",";
    json += "\"progress\":" + String(otaProgress) + ",";
    json += "\"error\":\"" + otaError + "\",";
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
    json += "\"staSsid\":\"" + String(staSsid) + "\",";
    // Passwort niemals im Klartext ausliefern: Frontend bekommt nur einen Marker,
    // dass eines gespeichert ist. Beim /wifi/save wird ein leeres Feld als
    // "bestehendes Passwort beibehalten" interpretiert (staPasswordKeep=1).
    json += "\"staPassword\":\"\",";
    json += "\"staPasswordSet\":" + String(staPassword[0] != '\0' ? "true" : "false") + ",";
    // WPA2-Enterprise (PEAP/MS-CHAPv2): Identity wird gespeichert und ausgeliefert,
    // sie ist nicht geheim (wird auf Funk-Ebene ohnehin sichtbar uebertragen).
    json += "\"staEnterprise\":" + String(staEnterprise ? "true" : "false") + ",";
    json += "\"staIdentity\":\"" + String(staIdentity) + "\",";
    json += "\"staAnonIdentity\":\"" + String(staAnonIdentity) + "\",";
    json += "\"staHostname\":\"" + String(staHostname) + "\",";
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
    json += "\"ntpServer\":\"" + String(ntpServer) + "\",";
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

    delay(500);  // Kurze Pause, damit Response gesendet wird
    ESP.restart();
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
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
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
        uint16_t newMinADC = server.arg("minADC").toInt();
        if (newMinADC >= 0 && newMinADC <= 1023) {
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
        uint16_t newMaxADC = server.arg("maxADC").toInt();
        if (newMaxADC >= 0 && newMaxADC <= 1023 && newMaxADC > autoBrightnessMinADC) {
            autoBrightnessMaxADC = newMaxADC;
            changed = true;
        }
    }

    // Min Helligkeit
    if (server.hasArg("minBrightness")) {
        uint8_t newMinBrightness = server.arg("minBrightness").toInt();
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
        uint8_t newMaxBrightness = server.arg("maxBrightness").toInt();
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
    DateTime rtcNow = rtc.now();
    unsigned long rtcTime = rtcNow.unixtime();
    
    // Hole letzte gespeicherte Zeit aus EEPROM
    unsigned long lastKnownTime = 0;
    lastKnownTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP)) << 24;
    lastKnownTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP + 1)) << 16;
    lastKnownTime |= ((unsigned long)EEPROM.read(ADDR_TIMESTAMP + 2)) << 8;
    lastKnownTime |= EEPROM.read(ADDR_TIMESTAMP + 3);
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println(  "║    STROMAUSFALL-PRÜFUNG (RTC)          ║");
    Serial.println(  "╚════════════════════════════════════════╝");
    
    Serial.printf("RTC-Zeit:             %lu\n", rtcTime);
    Serial.printf("Letzte bekannte Zeit: %lu\n", lastKnownTime);
    
    // Prüfe ob gültige Zeit gespeichert ist
    if (lastKnownTime < 1735689600) {  // Vor 2025-01-01
        Serial.println("→ Keine gültige Zeit gespeichert");
        Serial.println("✓ Erste Inbetriebnahme\n");
        
        // Übernehme RTC-Zeit als Startpunkt
        bootTime = rtcTime;
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
        bootTime = rtcTime;
        return true;
    }
    
    // Alles OK - normale Zeitdifferenz
    Serial.println("✓ Normale Zeitdifferenz");
    Serial.println("✓ Kein Stromausfall\n");
    
    // Übernehme RTC-Zeit als Basis
    bootTime = rtcTime;
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

    bool rtcAvailable = rtc.begin() && rtc.isrunning();
    #if (defined(USE_RTC) && USE_RTC == false)
        #warning "RTC not in use!"
        rtcAvailable = false;
    #endif

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

void setupWiFiStation() {
    if (!staEnabled || strlen(staSsid) == 0) {
        DEBUG_PRINTLN("→ WiFi Station deaktiviert (keine Credentials)");
        return;
    }

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
        struct station_config conf;
        memset(&conf, 0, sizeof(conf));
        strncpy((char*)conf.ssid, staSsid, sizeof(conf.ssid));
        wifi_station_set_config(&conf);

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
            wifi_station_set_enterprise_ca_cert((uint8_t*)wpa2_ca_cert, caLen);
        } else {
            DEBUG_PRINTLN("  CA-Cert:  <keiner – Server wird NICHT verifiziert>");
        }

        // Outer Identity (anonymous identity): konfigurierbar.
        // - Feld leer  -> set_enterprise_identity gar nicht aufrufen,
        //                  ESP-SDK sendet dann seinen Default
        //                  "anonymous@espressif.com" (valides user@realm).
        // - "anonymous" -> reiner String ohne Realm.
        // - "anonymous@schule.de" -> mit Realm, wie es viele Cisco-ISE-Setups
        //                            erwarten.
        if (strlen(staAnonIdentity) > 0) {
            DEBUG_PRINTF("  Outer:    %s\n", staAnonIdentity);
            wifi_station_set_enterprise_identity((uint8_t*)staAnonIdentity, strlen(staAnonIdentity));
        } else {
            DEBUG_PRINTLN("  Outer:    <SDK-Default 'anonymous@espressif.com'>");
        }
        wifi_station_set_enterprise_username((uint8_t*)staIdentity, strlen(staIdentity));
        wifi_station_set_enterprise_password((uint8_t*)staPassword, strlen(staPassword));

        wifi_station_connect();
    } else {
        WiFi.begin(staSsid, staPassword);
    }

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

        // WICHTIG: Auto-Reconnect deaktivieren, um AP-Funktionalität nicht zu beeinträchtigen
        WiFi.setAutoReconnect(false);
        WiFi.disconnect();

        // AP-Mode sicherstellen
        WiFi.mode(WIFI_AP_STA);  // Dual-Mode beibehalten
        DEBUG_PRINTLN("→ Auto-Reconnect deaktiviert, AP-Mode bleibt aktiv");
        DEBUG_PRINTLN("→ Verbinden Sie sich mit dem AP für neue WiFi-Einstellungen");
    }
}

// ════════════════════════════════════════════════════════════════
// NTP SETUP & SYNC
// ════════════════════════════════════════════════════════════════
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

    // Konfiguriere NTP (UTC speichern, Zeitzone wird beim Anzeigen umgerechnet)
    // Syntax: configTime(timezone_sec, daylight_sec, server1, server2, server3)
    configTime(0, 0, ntpServer, "pool.ntp.org", "time.nist.gov");

    DEBUG_PRINTLN("ℹ Zeitzone: UTC (Umrechnung auf MEZ/MESZ erfolgt automatisch)");

    DEBUG_PRINTLN("Warte auf NTP-Sync...");

    // Warte max. 5 Sekunden auf ersten Sync
    time_t now = time(nullptr);
    int timeout = 0;
    while (now < 1000000000 && timeout < 10) {  // Timestamp > ~2001
        delay(500);
        now = time(nullptr);
        DEBUG_PRINT(".");
        timeout++;
    }
    DEBUG_PRINTLN();

    if (now > 1000000000) {
        DEBUG_PRINTLN("✓ NTP-Sync erfolgreich!");
        DEBUG_PRINTF("  Unix-Zeit: %lld\n", (long long)now);

        struct tm* timeinfo = localtime(&now);
        DEBUG_PRINTF("  Datum: %04d-%02d-%02d %02d:%02d:%02d\n",
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec);

        // Übernehme NTP-Zeit als bootTime
        bootTime = now - (millis() / 1000);
        lastNtpSync = now;
        lastSyncTime = now;  // WICHTIG: Für Drift-Korrektur
        ntpSyncSuccessful = true;
        saveNTPConfig();  // Speichere letzten Sync
        saveDriftRate();  // Speichere auch lastSyncTime

        // Nächster Check in 1 Stunde
        nextNtpCheck = millis() + 3600000;
    } else {
        DEBUG_PRINTLN("⚠ NTP-Sync fehlgeschlagen (Timeout)");
        ntpSyncSuccessful = false;
        // Retry in 5 Minuten
        nextNtpCheck = millis() + 300000;
    }
}

void checkNTPSync() {
    // Nur wenn NTP aktiviert und WiFi verbunden
    if (!ntpEnabled || WiFi.status() != WL_CONNECTED) {
        return;
    }

    // Nur wenn Check-Zeit erreicht
    if (millis() < nextNtpCheck) {
        return;
    }

    DEBUG_PRINTLN("→ Stündlicher NTP-Sync...");

    time_t now = time(nullptr);
    if (now > 1000000000) {
        // Sync erfolgreich
        unsigned long oldBootTime = bootTime;
        bootTime = now - (millis() / 1000);

        // Berechne Drift seit letztem Sync
        if (lastNtpSync > 0) {
            long drift = (long)bootTime - (long)oldBootTime;
            DEBUG_PRINTF("  Drift seit letztem Sync: %ld Sekunden\n", drift);

            // Aktualisiere Drift-Rate (nutze bestehendes System!)
            unsigned long timeSinceSync = now - lastNtpSync;
            if (timeSinceSync > 3600) {  // Min. 1 Stunde
                float daysElapsed = timeSinceSync / 86400.0;
                float newDriftRate = drift / daysElapsed;

                if (syncCount > 0) {
                    driftRate = (driftRate * syncCount + newDriftRate) / (syncCount + 1);
                } else {
                    driftRate = newDriftRate;
                }

                syncCount++;
                saveDriftRate();
            }
        }

        lastNtpSync = now;
        lastSyncTime = now;  // WICHTIG: Für Drift-Korrektur
        ntpSyncSuccessful = true;
        saveNTPConfig();
        saveDriftRate();  // Speichere auch lastSyncTime

        DEBUG_PRINTLN("  ✓ NTP-Sync erfolgreich");
    } else {
        DEBUG_PRINTLN("  ⚠ NTP-Sync fehlgeschlagen");
        ntpSyncSuccessful = false;
    }

    // Nächster Check in 1 Stunde (oder 5 Min bei Fehler)
    nextNtpCheck = millis() + (ntpSyncSuccessful ? 3600000 : 300000);
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
    wasConnected = isConnected;
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
    
    if (rtc.begin())
    {
        if (!rtc.isrunning()) {
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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
    }

    // User-Helligkeit (0-80) auf LED-Helligkeit (0-204) mappen
    FastLED.setBrightness(map(brightness, 0, 80, 0, 204));

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
    FastLED.clear(true); 
    FastLED.setBrightness(0);
    yield();
    delay(1000);
}

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS MIT ADC (A0)
// ════════════════════════════════════════════════════════════════
unsigned long lastBrightnessUpdate = 0;
const unsigned long brightnessUpdateInterval = 10000;  // 2 Sekunden

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
    return;
    // ADC mehrfach auslesen und Mittelwert bilden (reduziert Rauschen)
    uint32_t adcSum = 0;
    const uint8_t samples = 10;
    for (uint8_t i = 0; i < samples; i++) {
        adcSum += analogRead(A0);
        delay(1);  // Kurze Pause zwischen Messungen
    }
    uint16_t adcValue = adcSum / samples;

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

    // Helligkeit setzen
    FastLED.setBrightness(newBrightness);

    DEBUG_PRINTF("Auto-Brightness: ADC=%d → Brightness=%d (Range: %d-%d)\n",
                 adcValue, newBrightness, autoBrightnessMin, autoBrightnessMax);
}

void loop()
{
    if (apActive)
    {
      dnsServer.processNextRequest();

      // LOGGING: Vor handleClient
      if (server.client() && server.client().available())
      {
        DEBUG_PRINTLN("→ Eingehender Request!");
      }

      server.handleClient();

      // WiFi Station überwachen (Auto-Reconnect)
      checkWiFiConnection();

      // NTP-Sync prüfen (stündlich)
      checkNTPSync();

      #if defined(DEBUG_MODE) && (DEBUG_MODE == false)
        #warning "WiFi AP: Timeout nur wenn keine Station konfiguriert"
        // AP-Timeout Strategie (Produktionsmodus):
        // - AP bleibt PERMANENT aktiv wenn WiFi Station konfiguriert ist (egal ob verbunden!)
        //   → Ermöglicht Neukonfiguration bei falschen Credentials
        // - AP schaltet nach 5 Min ab NUR wenn KEINE Station konfiguriert ist
        //   → Stromsparen im reinen AP-Modus
        //
        // WICHTIG: Zugriff für Konfiguration muss IMMER möglich sein!
        // Wenn staEnabled=true → Benutzer will Dual-Mode → AP muss erreichbar bleiben

        if (!staEnabled && millis() - apStartTime > AP_TIMEOUT)
        {
          WiFi.softAPdisconnect(true);
          WiFi.mode(WIFI_OFF);
          apActive = false;
          DEBUG_PRINTLN("⚠ AP nach Timeout deaktiviert (kein Dual-Mode)");
        }
      #else
        #warning "DEBUG_MODE is true, so WiFi AP bleibt immer aktiv"
      #endif
    }
    
  // Auto-Brightness aktualisieren (jede Sekunde)
  //updateBrightness();

  // Stromausfall-Modus: nur SOS rendern, normale Anzeige uebergehen.
  // Webserver und Auto-Reconnect laufen oben weiter, der User kann die
  // Zeit setzen und ueber /powerloss/clear quittieren.
  if (powerLossDetected) {
    renderPowerLossSOS();
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
