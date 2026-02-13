#ifndef _VARS_INC_
  #define _VARS_INC_

  #include <rgbPanel.h>
// ════════════════════════════════════════════════════════════════
// GLOBALE VARIABLEN
// ════════════════════════════════════════════════════════════════


unsigned long bootTime = 0;
unsigned long lastSyncTime = 0;
float driftRate = 0.0;
int syncCount = 0;

int8_t isConfigured = 0xFF;

const unsigned long AP_TIMEOUT = 300000;
bool powerLossDetected = false;  //Flag für Stromausfall

char AP_SSID[AP_SSID_LENGHT]     = "Wortuhr2026\0";
char AP_PASSWORD[AP_PASSWORD_LENGTH] = "Wortuhr2026\0";


uint16_t bootCounter = 0;

bool apActive                      = false;
unsigned long apStartTime          = 0;
static int lastDisplayedMinute     = -1;
unsigned long lastUpdateTime       = 0;
const unsigned long updateInterval = 1000; // 1 Sekunde

// ════════════════════════════════════════════════════════════════
// OTA UPDATE VARIABLEN
// ════════════════════════════════════════════════════════════════
char firmwareVersion[32] = "";
bool otaInProgress = false;
int otaProgress = 0;
String otaError = "";
unsigned long otaStartTime = 0;

// ════════════════════════════════════════════════════════════════
// WIFI STATION VARIABLEN
// ════════════════════════════════════════════════════════════════
char staSsid[32] = "";
char staPassword[64] = "";
bool staEnabled = false;
bool staDhcp = true;
IPAddress staIP(0, 0, 0, 0);
IPAddress staGateway(0, 0, 0, 0);
IPAddress staSubnet(255, 255, 255, 0);
IPAddress staDNS(0, 0, 0, 0);

// ════════════════════════════════════════════════════════════════
// NTP VARIABLEN
// ════════════════════════════════════════════════════════════════
char ntpServer[32] = "ptbtime1.ptb.de";
unsigned long lastNtpSync = 0;
bool ntpEnabled = false;
bool ntpSyncSuccessful = false;
unsigned long nextNtpCheck = 0;

// ════════════════════════════════════════════════════════════════
// AUTO-BRIGHTNESS VARIABLEN
// ════════════════════════════════════════════════════════════════
bool autoBrightnessEnabled = false;
uint16_t autoBrightnessMinADC = 200;   // Min ADC-Wert (ca. 0.2V bei 1024=1V)
uint16_t autoBrightnessMaxADC = 820;   // Max ADC-Wert (ca. 0.8V bei 1024=1V)
uint8_t autoBrightnessMin = 0;         // Minimale Helligkeit (bei wenig Licht)
uint8_t autoBrightnessMax = 80;        // Maximale Helligkeit (bei viel Licht) - 80 = 100%
const uint8_t MAX_BRIGHTNESS = 80;     // Maximum für Helligkeit (100%)

// ════════════════════════════════════════════════════════════════
// SPECIAL WORD VARIABLEN
// ════════════════════════════════════════════════════════════════
uint8_t specialWordInterval = 60;     // Anzeigeintervall in Minuten (Default: jede Stunde)

// ════════════════════════════════════════════════════════════════
// PATTERN TEST VARIABLEN
// ════════════════════════════════════════════════════════════════
bool patternTestRunning = false;      // Pattern-Test aktiv

// Intervall-Konstanten
#define INTERVAL_EVERY_MINUTE     1
#define INTERVAL_EVERY_5_MINUTES  5
#define INTERVAL_EVERY_10_MINUTES 10
#define INTERVAL_EVERY_15_MINUTES 15
#define INTERVAL_EVERY_30_MINUTES 30
#define INTERVAL_EVERY_60_MINUTES 60

//#include <html.inc>
#include <html.h>
#endif