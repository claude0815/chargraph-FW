#include <powerondetector.h>

// Flag setzen während System läuft
void setRunningFlag()
{
    EEPROM.write(ADDR_RUNNING_FLAG, RUNNING_FLAG_MAGIC);
    EEPROM.commit();
}

// Flag löschen bei sauberem Shutdown
void clearRunningFlag()
{
    EEPROM.write(ADDR_RUNNING_FLAG, 0x00);
    EEPROM.commit();
}

bool detectPowerLossFromResetReason()
{
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println(  "║   BOOT-GRUND ANALYSE                   ║");
    Serial.println(  "╚════════════════════════════════════════╝");
    
    rst_info* resetInfo = ESP.getResetInfoPtr();
    
    Serial.print("Reset Reason: ");
    Serial.println(resetInfo->reason);
    
    switch (resetInfo->reason) {
        case REASON_DEFAULT_RST:
            Serial.println("  → Power-On Reset");
            Serial.println("⚠ STROMAUSFALL oder erste Inbetriebnahme");
            return true;
            
        case REASON_WDT_RST:
            Serial.println("  → Watchdog Reset");
            Serial.println("⚠ System abgestürzt");
            return true;
            
        case REASON_EXCEPTION_RST:
            Serial.println("  → Exception Reset");
            Serial.println("⚠ Software-Fehler");
            return true;
            
        case REASON_SOFT_WDT_RST:
            Serial.println("  → Software Watchdog");
            Serial.println("⚠ System hing");
            return true;
            
        case REASON_SOFT_RESTART:
            Serial.println("  → Software Restart");
            Serial.println("✓ Normaler Software-Neustart");
            return false;
            
        case REASON_DEEP_SLEEP_AWAKE:
            Serial.println("  → Deep Sleep Awake");
            Serial.println("✓ Aufwachen aus Deep Sleep");
            return false;
            
        case REASON_EXT_SYS_RST:
            Serial.println("  → External Reset (Button)");
            Serial.println("✓ Reset-Taste gedrückt");
            return false;
            
        default:
            Serial.println("  → Unbekannt");
            return false;
    }
    
    Serial.println("════════════════════════════════════════\n");
}

bool detectPowerLossWithoutRTC()
{
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println(  "║    STROMAUSFALL-PRÜFUNG (ohne RTC)     ║");
    Serial.println(  "╚════════════════════════════════════════╝");
    
    // Prüfe ob Running-Flag gesetzt war
    uint8_t runningFlag = EEPROM.read(ADDR_RUNNING_FLAG);
    
    if (runningFlag == RUNNING_FLAG_MAGIC) {
        // Flag war gesetzt → System lief und wurde abrupt unterbrochen
        Serial.println("\n╔═════════════════════════════════════════════╗");
        Serial.println(  "║          ⚠⚠⚠ STROMAUSFALL ERKANNT!          ║");
        Serial.println(  "║  System wurde nicht sauber heruntergefahren ║");
        Serial.println(  "╚═════════════════════════════════════════════╝\n");
        
        // Lösche Flag (wird später wieder gesetzt wenn System läuft)
        EEPROM.write(ADDR_RUNNING_FLAG, 0x00);
        EEPROM.commit();
        
        return true;
    } else {
        Serial.println("\n╔═════════════════════════════════════════════╗");
        Serial.println  ("║  ✓ Normaler Start (Flag nicht gesetzt)      ║");
        Serial.println(  "║    oder erste Inbetriebnahme                ║");
        Serial.println(  "╚═════════════════════════════════════════════╝\n");
        return false;
    }
}

void checkPowerLoss()
{
    // Lade Boot-Counter
    bootCounter = EEPROM.read(ADDR_BOOT_COUNTER) << 8;
    bootCounter |= EEPROM.read(ADDR_BOOT_COUNTER + 1);
    
    // Prüfe Clean-Shutdown-Flag
    bool cleanShutdown = EEPROM.read(ADDR_CLEAN_SHUTDOWN) == 0xAA;
    
    bootCounter++;
    
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN(  "║   BOOT-ANALYSE                         ║");
    DEBUG_PRINTLN(  "╚════════════════════════════════════════╝");
    DEBUG_PRINTF("\nBoot #%d\n", bootCounter);
    
    if (cleanShutdown) {
        DEBUG_PRINTLN("✓ Letzter Shutdown war sauber (Timeout)");
        EEPROM.write(ADDR_CLEAN_SHUTDOWN, 0x00);  // Reset
    } else {
        DEBUG_PRINTLN("⚠ STROMAUSFALL ERKANNT!");
        DEBUG_PRINTLN("  (Kein Clean-Shutdown-Flag)");
    }
    
    // Speichere neuen Boot-Counter
    EEPROM.write(ADDR_BOOT_COUNTER, (bootCounter >> 8) & 0xFF);
    EEPROM.write(ADDR_BOOT_COUNTER + 1, bootCounter & 0xFF);
    EEPROM.commit();
    
    DEBUG_PRINTLN("════════════════════════════════════════\n");
}
