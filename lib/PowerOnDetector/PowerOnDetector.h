#ifndef _POWERONDETECTOR_H_
  #define _POWERONDETECTOR_H_
  #include <Arduino.h>
  //#include <FastLED.h>
  #include <EEPROM.h>
  //#include <RTClib.h>
  #include <user_interface.h>  // Für rst_info
  #include <extern.inc>
  #include <defines.inc>
  //#include <vars.inc>
  extern bool detectPowerLossWithoutRTC();
  extern void setRunningFlag();
  extern void clearRunningFlag();
  extern bool detectPowerLossFromResetReason();
  extern void powerLossLoop();
  extern void checkPowerLoss();
  extern uint16_t bootCounter;
#endif