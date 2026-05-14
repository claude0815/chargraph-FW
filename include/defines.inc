#ifndef _DEFINES_INC_
  #define _DEFINES_INC_

#ifdef BRIDGE_LEDS_POS77
   #warning "BRIDGE_LEDS_POS77 is definded, so RGB-LED on pos 77 would bridged"
#endif

#if defined(DEBUG_SOURCE) && (DEBUG_SOURCE == true)
  #warning "DEBUG_MODE is set to true, so Serial is active"
  #define DEBUG_PRINT(x) {Serial.print(x);Serial.flush(); yield();}
  #define DEBUG_PRINTLN(x) {Serial.println(x);Serial.flush(); yield();}
  #define DEBUG_PRINTF(...) {Serial.printf(__VA_ARGS__);Serial.flush(); yield();}
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#define I2C_SDA D2
#define I2C_SCL D1

// APA102/SK9822 auf D7 (DATA), D5 (CLK)
#define DATA_PIN2  D7
#define CLK_PIN2   D5

#define AP_SSID_LENGHT     13
#define AP_PASSWORD_LENGTH 17

// ════════════════════════════════════════════════════════════════
// EEPROM ADRESSEN
// ════════════════════════════════════════════════════════════════
#define EEPROM_SIZE         1024
#define ADDR_BRIGHTNESS       0
#define ADDR_COLOR_R          1
#define ADDR_COLOR_G          2
#define ADDR_COLOR_B          3
#define ADDR_SPECIAL_R        4
#define ADDR_SPECIAL_G        5
#define ADDR_SPECIAL_B        6
#define ADDR_CONFIGURED       7
#define ADDR_TIMESTAMP        8
#define ADDR_SSID             (ADDR_TIMESTAMP+sizeof(savedTime))
#define ADDR_PASSWORD         (ADDR_SSID + AP_SSID_LENGHT)
#define ADDR_WIFI_SET         (ADDR_PASSWORD + AP_PASSWORD_LENGTH)
#define ADDR_LAST_SYNC       40
#define ADDR_DRIFT_RATE      44
#define ADDR_SYNC_COUNT      48
#define ADDR_CHARSOAP        50
#define ADDR_CHARSOAP_SET   161
#define ADDR_BOOT_COUNTER   163    // Boot-Counter
#define ADDR_CLEAN_SHUTDOWN 164    // Clean-Shutdown-Flag
#define ADDR_RUNNING_FLAG   165    // "System läuft" Flag

// ════════════════════════════════════════════════════════════════
// OTA UPDATE ADRESSEN (346 bytes frei: 166-511)
// ════════════════════════════════════════════════════════════════
#define ADDR_OTA_VERSION    166    // Firmware-Version (32 bytes)
#define ADDR_OTA_BUILD_DATE 198    // Build-Datum/Zeit (20 bytes)
#define ADDR_OTA_FLAGS      218    // OTA-Status-Flags (1 byte)

#define OTA_FLAG_UPDATE_SUCCESS 0x01
#define OTA_FLAG_UPDATE_FAILED  0x02

// ════════════════════════════════════════════════════════════════
// WIFI STATION & NTP ADRESSEN (293 bytes frei: 219-511)
// ════════════════════════════════════════════════════════════════
#define ADDR_STA_SSID           219    // Station SSID (32 bytes)
#define ADDR_STA_PASSWORD       251    // Station Password (64 bytes)
#define ADDR_STA_ENABLED        315    // Station enabled flag (1 byte)
#define ADDR_STA_DHCP           316    // DHCP enabled (1 byte)
#define ADDR_STA_IP             317    // Static IP (4 bytes)
#define ADDR_STA_GATEWAY        321    // Gateway (4 bytes)
#define ADDR_STA_SUBNET         325    // Subnet mask (4 bytes)
#define ADDR_STA_DNS            329    // DNS server (4 bytes)
#define ADDR_NTP_SERVER         333    // NTP server hostname (32 bytes)
#define ADDR_NTP_LAST_SYNC      365    // Last NTP sync timestamp (4 bytes)
#define ADDR_NTP_ENABLED        369    // NTP enabled flag (1 byte)

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS ADRESSEN (140 bytes frei: 377-511)
// ════════════════════════════════════════════════════════════════
#define ADDR_AUTO_BRIGHTNESS_ENABLED  370    // Auto-Brightness aktiviert (1 byte)
#define ADDR_AUTO_BRIGHTNESS_MIN_ADC  371    // Min ADC-Wert (2 bytes uint16_t)
#define ADDR_AUTO_BRIGHTNESS_MAX_ADC  373    // Max ADC-Wert (2 bytes uint16_t)
#define ADDR_AUTO_BRIGHTNESS_MIN      375    // Min Helligkeit (1 byte uint8_t)
#define ADDR_AUTO_BRIGHTNESS_MAX      376    // Max Helligkeit (1 byte uint8_t)

// ════════════════════════════════════════════════════════════════
// SPECIAL WORD & MINUTE LEDS ADRESSEN (93 bytes frei: 377-511)
// ════════════════════════════════════════════════════════════════
#define ADDR_SPECIAL_WORD_1       377    // Erstes Spezialwort (12 bytes)
#define ADDR_SPECIAL_WORD_2       389    // Zweites Spezialwort (12 bytes)
#define ADDR_SPECIAL_WORD_3       401    // Drittes Spezialwort (12 bytes)
#define ADDR_MINUTE_LEDS          413    // 4 Minuten-LEDs Positionen (4 bytes)
#define ADDR_SPECIAL_WORD_SET     417    // Flag: Custom Special Words (1 byte)
#define ADDR_MINUTE_LEDS_SET      418    // Flag: Custom Minute LEDs (1 byte)
#define ADDR_SPECIAL_WORD_INTERVAL 419   // Anzeigeintervall in Minuten (1 byte)
#define ADDR_SPECIAL_WORD_MODE    613    // Anzeige-Modus (0=off, 1=interval, 2=parallel)
#define ADDR_USE_RAINBOW          614    // Regenbogenfarben fuer Uhrzeit (1 byte, Magic)
#define RAINBOW_MAGIC             0xA5

#define SPECIAL_WORD_MODE_OFF      0
#define SPECIAL_WORD_MODE_INTERVAL 1
#define SPECIAL_WORD_MODE_PARALLEL 2

// ════════════════════════════════════════════════════════════════
// WPA2-ENTERPRISE (PEAP/MS-CHAPv2) ADRESSEN (frei: 485-511)
// ════════════════════════════════════════════════════════════════
#define ADDR_STA_ENTERPRISE_ENABLED  420   // Enterprise aktiv (1 byte, Magic)
#define ADDR_STA_ENTERPRISE_IDENTITY 421   // Inner Identity / Benutzername (64 bytes)
#define ADDR_STA_ENTERPRISE_ANON_ID  485   // Outer/Anonymous Identity (64 bytes)
#define ADDR_STA_HOSTNAME            549   // DHCP-Hostname (64 bytes)

#define STA_ENABLED_MAGIC       0xAA
#define NTP_ENABLED_MAGIC       0xBB
#define AUTO_BRIGHTNESS_MAGIC   0xCC
#define SPECIAL_WORD_MAGIC      0xDD
#define MINUTE_LEDS_MAGIC       0xEE
#define STA_ENTERPRISE_MAGIC    0xE1

#define RUNNING_FLAG_MAGIC 0x42    // Magic Byte
#define MAGIC_BYTE_INIT    0x55

#endif