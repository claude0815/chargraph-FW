# CharGraph-FW - ESP8266 WiFi-Steuerbare Zeichengrafik

Eine vollständige Firmware für eine WiFi-gesteuerte LED-Matrix-Wort-Anzeige auf Basis des ESP8266. Das Projekt zeigt Wörter in beleuchteten Buchstaben an und bietet eine Web-basierte Konfigurationsoberfläche.

## 📋 Inhaltsverzeichnis

- [Projektübersicht](#projektübersicht)
- [Verzeichnisstruktur](#verzeichnisstruktur)
- [Installation und Setup](#installation-und-setup)
- [Build-System](#build-system)
- [Verwendete Bibliotheken](#verwendete-bibliotheken)
- [Hardware-Konfiguration](#hardware-konfiguration)
- [Architektur](#architektur)
- [Konfiguration](#konfiguration)
- [Entwicklung](#entwicklung)
- [Bekannte Probleme und Tipps](#bekannte-probleme-und-tipps)

---

## 🎯 Projektübersicht

**CharGraph-FW** ist eine ESP8266-basierte Firmware für ein 18x18 LED-Matrix-Display. Das Projekt erzeugt eine WiFi-steuerbare Wort-Uhr, die die Zeit durch beleuchtete Buchstaben anzeigt.

### Hauptfunktionen

- **WiFi-Konnektivität**: Access Point (AP) und Station (STA) Modi
- **Zeitanzeige**: NTP-Synchronisation mit deutschem PTB-Zeitserver
- **Web-Interface**: Responsive HTML-Oberfläche zur Konfiguration
- **EEPROM-Speicher**: Persistente Speicherung von Einstellungen
- **WS2812B-Support**: Programmierbare RGB-LED-Ansteuerung
- **RTC-Unterstützung**: Optional DS1307 Real-Time Clock
- **OTA-Updates**: Over-The-Air Firmware-Updates
- **Mehrsprachigkeit**: Unterstützung für benutzerdefinierte Zeichen-Muster

### Zielplattformen

- **ESP8266** (ESP-12E/F kompatibel) - Standardboard
- **D1 Mini** - Alternative Board-Option

---

## 📁 Verzeichnisstruktur

```
CharGraph-FW/
├── src/
│   └── main.cpp                    # Hauptanwendungsdatei
├── include/
│   ├── common.inc                  # Zentrale Include-Datei für alle Abhängigkeiten
│   ├── defines.inc                 # Hardware-Definitionen und Debug-Makros
│   ├── vars.inc                    # Globale Variablendeklarationen
│   ├── extern.inc                  # Externe Deklarationen für globale Variablen
│   ├── html.h                      # Auto-generierte HTML-Header (GZIP-komprimiert)
│   └── [weitere Header-Dateien]
├── lib/
│   ├── rgbPanel/                   # RGB-Panel Bibliothek (11x11 LED-Matrix)
│   │   ├── src/
│   │   ├── examples/
│   │   └── library.json
│   ├── CharGraphTimeLogic/         # Zeit-Logik und Wort-Matching
│   │   ├── src/
│   │   ├── examples/
│   │   ├── blackboxtest/           # Unit-Tests
│   │   └── library.json
│   ├── PowerOnDetector/            # Stromausfalls-Erkennung
│   │   ├── src/
│   │   └── library.json
│   └── info/                       # Status-Anzeige Informationen
│       ├── src/
│       └── library.json
├── html/
│   └── index.html                  # Web-Interface HTML-Quelle
├── doc/                            # Dokumentation
├── .vscode/                        # VS Code Einstellungen
├── .platformio.ini                 # Projekt-Konfiguration
├── html_to_header.py               # Pre-Build Script für HTML-Konvertierung
└── README.md                       # Diese Datei
```

### Verzeichnis-Beschreibungen

| Verzeichnis | Beschreibung |
|---|---|
| `src/` | Quellcode der Hauptanwendung |
| `include/` | Header-Dateien und Definitionen |
| `lib/` | Lokale Bibliotheken und Module |
| `html/` | Web-Interface Quellen |
| `doc/` | Zusätzliche Dokumentation |
| `.vscode/` | VS Code Editor-Konfiguration |
| `.pio/` | PlatformIO Build-Artefakte (nicht im Repo) |
| `.git/` | Git-Versionskontrolle |

---

## 🚀 Installation und Setup

### Voraussetzungen

- **PlatformIO**: Installiert (als VS Code Extension oder CLI)
- **Python 3.6+**: Für das `html_to_header.py` Script
- **USB-zu-UART Adapter**: Zum Hochladen auf ESP8266

### Schritt 1: Repository klonen

```bash
git clone <repository-url>
cd CharGraph-FW
```

### Schritt 2: Abhängigkeiten aktualisieren

```bash
pio lib update
```

### Schritt 3: Serial Port konfigurieren

Die `platformio.ini` enthält vordefinierte Ports:
- **esp8266clone**: `upload_port = COM13`
- **d1_mini**: `upload_port = COM5`

Diese müssen ggf. an die lokale Umgebung angepasst werden.

### Schritt 4: Projekt bauen und hochladen

```bash
# Bauen und hochladen (Standard: esp8266clone)
pio run --target upload

# Oder für D1 Mini:
pio run -e d1_mini --target upload
```

---

## 🔨 Build-System

### PlatformIO Befehle

```bash
# Projekt kompilieren
pio run

# Kompilieren und hochladen
pio run --target upload

# Für spezifische Umgebung bauen
pio run -e esp8266clone
pio run -e d1_mini

# Clean Build durchführen
pio run --target clean

# Serielle Überwachung starten
pio device monitor

# Abhängigkeiten aktualisieren
pio lib update
```

### Build-Flags (platformio.ini)

Die Datei `platformio.ini` definiert wichtige Compile-Flags:

```ini
build_flags =
    -I include
    -DPIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY
    -DDEBUG_ESP_PORT=Serial
    -DDEBUG_MODE=false              # true = Debug, false = Produktion
    -DUSE_RTC=false                 # RTC DS1307 Unterstützung
    -DTEST_RGB_ONLY=false           # RGB-Only Test-Modus
    -DPOWERLOSSDETECT=false         # Stromausfalls-Detektion
```

### Umgebungen

#### esp8266clone (Standard)

- **Board**: ESP-12E kompatibel
- **CPU-Frequenz**: 160 MHz
- **Flash-Modus**: DIO (kompatibler für Clones)
- **Upload-Port**: COM13
- **Upload-Geschwindigkeit**: 115200 baud

```ini
[env:esp8266clone]
board = esp12e
upload_speed = 115200
upload_port = COM13
board_build.flash_mode = dio
board_build.f_cpu = 160000000L
board_build.ldscript = eagle.flash.4m2m.ld
```

#### d1_mini

- **Board**: Wemos D1 Mini
- **CPU-Frequenz**: 160 MHz
- **Flash-Modus**: DIO
- **Upload-Port**: COM5
- **Upload-Geschwindigkeit**: 115200 baud

```ini
[env:d1_mini]
board = d1_mini
upload_speed = 115200
upload_port = COM5
board_build.flash_mode = dio
board_build.f_cpu = 160000000L
board_build.ldscript = eagle.flash.4m2m.ld
```

### Monitor-Einstellungen

```ini
[common]
monitor_speed = 115200
monitor_dtr = 1                     # RTS/DTR für Auto-Reset
monitor_rts = 1
monitor_filters =
    default
    time
    esp8266_exception_decoder
```

---

## 📦 Verwendete Bibliotheken

### Externe Abhängigkeiten

Die Bibliotheken werden in `platformio.ini` unter `lib_deps` definiert:

```ini
lib_deps =
    fastled/FastLED @ ^3.6.0
    adafruit/RTClib @ ^2.1.1
```

#### FastLED 3.6.0+

- **Zweck**: LED-Steuerung und Animation für WS2812B RGB-LEDs
- **Dokumentation**: [FastLED GitHub](https://github.com/FastLED/FastLED)
- **Hauptfunktionen**:
  - Unterstützung für verschiedene LED-Chipsets
  - Color-Matching und HSV-Support
  - Brightness-Control

#### RTClib 2.1.1+ (von Adafruit)

- **Zweck**: Unterstützung für DS1307 Real-Time Clock Module
- **Optional**: Kann deaktiviert werden mit `USE_RTC=false`
- **Dokumentation**: [Adafruit GitHub](https://github.com/adafruit/RTClib)

### Eingebaute ESP8266 Bibliotheken

Die folgenden Bibliotheken sind Teil des Arduino-Frameworks für ESP8266:

| Bibliothek | Zweck |
|---|---|
| `ESP8266WiFi` | WiFi-Konnektivität |
| `ESP8266WebServer` | HTTP-Server für Web-Interface |
| `DNSServer` | DNS-Datenbank für Captive Portal |
| `EEPROM` | Permanente Speicherung |
| `Wire` | I2C-Kommunikation (RTC) |
| `ArduinoJson` | JSON-Verarbeitung |

### Lokale Bibliotheken (in `lib/`)

#### rgbPanel

- **Pfad**: `lib/rgbPanel/`
- **Beschreibung**: Verwaltung der 11x11 (121 LED) LED-Matrix
- **Hauptfunktionen**:
  - Wort-Suche und -Hervorhebung
  - Zeit-Display mit speziellen Übergängen
  - Farb- und Helligkeitskontrolle
- **Hardware**: WS2812B LEDs auf Pin D6

#### CharGraphTimeLogic

- **Pfad**: `lib/CharGraphTimeLogic/`
- **Beschreibung**: Zeit-Logik und Wort-Matching Algorithmen
- **Enthält**: Unit-Tests im `blackboxtest/` Verzeichnis
- **Hauptfunktionen**:
  - Zeitformat-Konvertierung
  - Wort-Matching für verschiedene Zeitzonen

#### PowerOnDetector

- **Pfad**: `lib/PowerOnDetector/`
- **Beschreibung**: Erkennung von Stromausfällen und Neustarts
- **Funktionen**:
  - Boot-Counter-Verwaltung
  - System-State-Persistenz

#### info

- **Pfad**: `lib/info/`
- **Beschreibung**: Verbindungsstatus und Informationsanzeige
- **Funktionen**:
  - Status-Display-Funktionen

---

## 🖥️ Hardware-Konfiguration

### Zielplattform: ESP8266

| Eigenschaft | Wert |
|---|---|
| Prozessor | 32-bit Xtensa |
| CPU-Frequenz | 160 MHz (eingestellt) |
| RAM | 160 KB |
| Flash | 4 MB (2 MB Sketch + 2 MB OTA) |
| WiFi | 802.11b/g/n |

### Pin-Belegung

| Pin | Funktion | Standard | Alternativ |
|---|---|---|---|
| D6 | RGB LED Data | WS2812B | - |
| D7 | APA102/SK9822 Data | - | APA102 |
| D5 | APA102/SK9822 Clock | - | APA102 |
| D1 (GPIO5) | I2C SCL | RTC/DS1307 | - |
| D2 (GPIO4) | I2C SDA | RTC/DS1307 | - |
| RX | Serieller Input | - | - |
| TX | Serieller Output | - | Debugging |

### LED-Matrix

- **Typ**: 11x11 addressierbare RGB-LED-Matrix
- **LED-Chip**: WS2812B (NeoPixel)
- **Gesamtzahl LEDs**: 121
- **Stromversorgung**: 5V
- **Datenleitung**: D6 (GPIO12)
- **Maximale Stromaufnahme**: ~7,3 A (bei voller Helligkeit, alle weiß)

### RTC-Modul (optional)

- **Chip**: DS1307
- **Interface**: I2C
- **Addresse**: 0x68
- **Spannungsversorgung**: 3.3V
- **SDA**: D2 (GPIO4)
- **SCL**: D1 (GPIO5)

---

## 🏗️ Architektur

### Kernel-Struktur

Das Projekt folgt einer modularen Architektur mit einem zentralen Include-System:

```
main.cpp
  ↓
include/common.inc (zentrale Include-Datei)
  ├── include/defines.inc
  ├── include/vars.inc
  ├── include/extern.inc
  └── [weitere Header-Dateien]
```

#### Kernkomponenten

| Datei | Zweck |
|---|---|
| `src/main.cpp` | Hauptanwendungseinstiegspunkt mit `setup()` und `loop()` |
| `include/common.inc` | Zentrale Include-Datei für alle Abhängigkeiten |
| `include/defines.inc` | Hardware-Definitionen, Debug-Makros, EEPROM-Adressen |
| `include/vars.inc` | Globale Variablendeklarationen |
| `include/extern.inc` | Externe Variablendeklarationen |

### WiFi-Modi

Das System unterstützt drei Betriebsmodi:

#### 1. Access Point Mode (AP)

- **Verwendung**: Initiale Konfiguration und Setup
- **Features**:
  - Captive Portal für automatische Umleitung
  - SSID: Konfigurierbar (z.B. "CharGraph")
  - Passwort: Konfigurierbar
  - IP-Adresse: Standard 192.168.4.1

#### 2. Station Mode (STA)

- **Verwendung**: Normalbetrieb mit bestehender WiFi-Verbindung
- **Features**:
  - Automatische Verbindung zu vorgespeichertem WiFi
  - DHCP-IP-Zuweisung
  - NTP-Zeitsynchronisation

#### 3. Dual Mode

- **Verwendung**: Gleichzeitiger Betrieb als AP und STA
- **Features**:
  - Konfiguration über AP möglich während STA-Betrieb
  - Flexible Netzwerkverwaltung

### Zeitsynchronisation

#### NTP (Network Time Protocol)

- **Server**: Deutsches PTB (`ptbtime1.ptb.de`)
- **Update-Intervall**: Konfigurierbar
- **Timezone**: Mitteleuropäische Zeit (CET/CEST)
- **DST-Unterstützung**: Automatische Sommerzeit-Berechnung

#### RTC-Fallback

- **Modul**: DS1307 (optional)
- **Verwendung**: Falls NTP nicht verfügbar
- **I2C-Adresse**: 0x68

#### Drift-Korrektur

- **Mechanismus**: Kontinuierliche Drift-Messung und Korrektur
- **Speicherung**: EEPROM-basierte Kalibrierung

### EEPROM-Memory-Layout

Das System nutzt ein ausgefeiltes EEPROM-Layout für persistente Speicherung:

| Bereich | Inhalt | Größe |
|---|---|---|
| 0-511 | Konfiguration (Helligkeit, Farben, WiFi) | 512 B |
| 512-1023 | Zeit-Management (Boot-Zeit, Drift, Sync) | 512 B |
| 1024-1535 | Netzwerk-Einstellungen | 512 B |
| 1536-2047 | Auto-Brightness-Kalibrierung | 512 B |
| 2048+ | Character Pattern Storage (charsoap) | Variabel |

Die genauen Adressen sind in `include/defines.inc` definiert.

---

## ⚙️ Konfiguration

### html_to_header.py - HTML zu Header Konvertierung

**Zweck**: Das Pre-Build-Script konvertiert die HTML-Datei in einen komprimierten C-Header für die Einbettung in die Firmware.

#### Funktionsweise

```python
# Eingabe: html/index.html
# Ausgabe: include/html.h
```

1. **HTML-Datei lesen**: Liest `html/index.html` als UTF-8 Text
2. **GZIP-Kompression**: Komprimiert den HTML-Inhalt mit Kompressionsebene 9
3. **Hex-Array-Generierung**: Wandelt komprimierte Bytes in C-Array um
4. **Header-Datei schreiben**: Erzeugt `include/html.h` mit:
   - Größenangaben (Original vs. komprimiert)
   - Kompressionsquote
   - `HTML_PAGE_GZIP[]` Array
   - `HTML_PAGE_GZIP_LEN` Konstante

#### Script-Details

```python
import os
import gzip

html_path = "html/index.html"
header_path = "include/html.h"

# HTML lesen und komprimieren
with open(html_path, 'r', encoding='utf-8') as f:
    html_content = f.read()

html_bytes = html_content.encode('utf-8')
compressed = gzip.compress(html_bytes, compresslevel=9)

# Größen berechnen
original_size = len(html_bytes)
compressed_size = len(compressed)
ratio = (1 - compressed_size / original_size) * 100

# Header-Datei generieren mit Komprimierungsstatistik
header_content = f"""// Auto-generiert von html_to_header.py
// Original-Größe:     {original_size} Bytes
// Komprimierte Größe: {compressed_size} Bytes
// Kompression:        {ratio:.1f}%

#ifndef HTML_H
#define HTML_H

#include <Arduino.h>

// GZIP-komprimierte HTML-Seite
const uint8_t HTML_PAGE_GZIP[] PROGMEM = {{
    // ... Hex-Daten ...
}};

const size_t HTML_PAGE_GZIP_LEN = {compressed_size};

#endif // HTML_H
"""
```

#### Integration in Build-Prozess

Das Script wird automatisch vor jedem Build ausgeführt:

```ini
extra_scripts = pre:html_to_header.py
```

**Wichtig**: Änderungen an `html/index.html` werden automatisch beim nächsten Build in die Firmware übernommen.

### Web-Interface (html/index.html)

Das Web-Interface bietet folgende Konfigurationsmöglichkeiten:

- **WiFi-Einstellungen**: SSID und Passwort für Station-Modus
- **Farb-Kontrolle**: RGB-Farbauswahl für LED-Anzeige
- **Helligkeits-Einstellungen**: LED-Helligkeit anpassen
- **Zeichen-Muster**: Upload benutzerdefinierter Zeichenmuster
- **System-Information**: Anzeige von Firmware-Version und Status

---

## 👨‍💻 Entwicklung

### Debug-Modus

Debug-Ausgabe wird durch Makros in `include/defines.inc` kontrolliert:

```c
DEBUG_PRINT(x)      // Ausgabe ohne Zeilenumbruch
DEBUG_PRINTLN(x)    // Ausgabe mit Zeilenumbruch
DEBUG_PRINTF(...)   // Formatierte Ausgabe
```

#### Debug-Flag aktivieren

1. In `platformio.ini` ändern:
   ```ini
   -DDEBUG_MODE=true
   ```

2. Projekt neu bauen:
   ```bash
   pio run --target clean
   pio run --target upload
   ```

3. Serielle Ausgabe überwachen:
   ```bash
   pio device monitor
   ```

### Serieller Monitor

```bash
# Mit Standard-Einstellungen starten
pio device monitor

# Mit Filtern (Zeit-Anzeige, Exception-Dekoder)
pio device monitor --filter=default --filter=time --filter=esp8266_exception_decoder
```

### Lokale Bibliotheken entwickeln

Jede Bibliothek in `lib/` hat eine `library.json`:

```json
{
  "name": "rgbPanel",
  "version": "1.0.0",
  "description": "RGB Panel control library",
  "keywords": ["LED", "RGB", "FastLED"],
  "authors": [{"name": "Author Name"}],
  "license": "MIT",
  "frameworks": ["arduino"],
  "platforms": ["espressif8266"]
}
```

### Unit-Tests

Für `CharGraphTimeLogic` sind Unit-Tests vorhanden:

```bash
# Tests ausführen (wenn unterstützt)
cd lib/CharGraphTimeLogic
# Weitere Anweisungen siehe in der Bibliothek
```

---

## 🔄 OTA (Over-The-Air) Updates

Das System unterstützt Firmware-Updates über WiFi:

### Features

- **Progress Tracking**: Echtzeit-Update-Fortschritt
- **Versionsverwaltung**: Firmware-Version im EEPROM
- **Rollback-Mechanismus**: Fallback auf vorherige Version
- **Sicherheit**: Prüfsummen-Verifikation

### Update-Prozess

1. Neue Firmware über Web-Interface hochladen
2. ESP8266 validiert Update
3. Firmware wird geschrieben
4. System startet mit neuer Version neu

---

## 🐛 Bekannte Probleme und Tipps

### ESP8266 Clone Stabilität

**Problem**: Upload-Fehler bei ESP-12E Clones

**Lösungen**:
1. `flash_mode = dio` statt `qio` (bereits eingestellt)
2. `upload_speed = 115200` verwenden (nicht höher)
3. `monitor_dtr = 1` und `monitor_rts = 1` aktivieren
4. USB-Kabel kurz halten oder hochwertiges Kabel verwenden
5. Externe 5V Stromversorgung verwenden

### WiFi-Verbindungsprobleme

**Problem**: Frequent WiFi-Trennungen

**Lösungen**:
1. Stromversorgung des ESP8266 prüfen (mindestens 500mA)
2. WiFi-Kanal in AP-Modus wechseln
3. RTC-Kalibrierung überprüfen
4. EEPROM-Kalibrierung zurücksetzen

### Speicherlecks

**Debug-Tipp**: Heap-Größe in `loop()` monitoren:

```c
DEBUG_PRINTF("Free heap: %d bytes\n", ESP.getFreeHeap());
```

### LED-Probleme

**Problem**: Einzelne oder alle LEDs zeigen falsche Farben

**Lösungen**:
1. Pin D6 auf Kurzschlüsse prüfen
2. Stromversorgung der LED-Matrix überprüfen (5V, GND)
3. Daten-Line Widerstand (330-470 Ohm) überprüfen
4. `TEST_RGB_ONLY=true` Flag zum Testen verwenden

### RTC-Kommunikation

**Problem**: RTC wird nicht erkannt

**Überprüfung**:
1. I2C-Pins D1 (SCL) und D2 (SDA) verbunden?
2. Pull-Up-Widerstände vorhanden? (4.7k Ohm)
3. I2C-Adresse korrekt? (0x68 für DS1307)
4. `USE_RTC=true` in `platformio.ini` gesetzt?

### Zeitsynchronisation

**Problem**: Zeit wird nicht über NTP synchronisiert

**Überprüfung**:
1. WiFi-Verbindung aktiv?
2. Internetzugang vorhanden?
3. NTP-Server erreichbar? (`ptbtime1.ptb.de`)
4. Firewall blockiert Port 123?
5. System-Zeit mit RTC als Fallback verwenden

---

## 📚 Zusätzliche Ressourcen

### Offizielle Dokumentation

- **ESP8266 Arduino Core**: [GitHub](https://github.com/esp8266/Arduino)
- **FastLED Library**: [GitHub](https://github.com/FastLED/FastLED)
- **PlatformIO**: [Dokumentation](https://docs.platformio.org)
- **Arduino Reference**: [arduino.cc](https://www.arduino.cc/reference)

### Community und Support

- **ESP8266 Community**: [esp8266.com](https://esp8266.com)
- **Arduino Forum**: [forum.arduino.cc](https://forum.arduino.cc)
- **GitHub Issues**: Fehlerberichte und Feature-Anfragen

### Weitere Dateien

- `doc/`: Zusätzliche Dokumentation

---

## 👨‍🔧 Beitragen

Beiträge sind willkommen! Bitte erstellen Sie einen Pull Request mit:

1. Beschreibung der Änderungen
2. Tests für neue Funktionen
3. Dokumentation-Updates
4. Hinweis auf zugehörige Issues

---

## 📞 Support und Kontakt

Bei Fragen oder Problemen:

1. Erstellen Sie einen GitHub Issue mit:
   - Fehlerbeschreibung
   - Build-Ausgabe
   - Serielles Debugging-Output

---

**Zuletzt aktualisiert**: 2026-01-25
**Version**: 1.0
**Entwicklungsumgebung**: PlatformIO + VS Code
