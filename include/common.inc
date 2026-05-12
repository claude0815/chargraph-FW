#ifndef _COMMON_INC_
  #define _COMMON_INC_
  #include <Arduino.h>
  #include <FastLED.h>
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <DNSServer.h>
  #include <EEPROM.h>
  #include <Wire.h>
  #include <RTClib.h>
  #include <ESP8266httpUpdate.h>
  #include <time.h>  // NTP support

  #include <user_interface.h>  // Für rst_info
  extern "C" {
    #include <wpa2_enterprise.h>  // PEAP / MS-CHAPv2 Auth
  }

  #include <extern.inc>
  #include <defines.inc>
  #include <version.inc>
  #include <vars.inc>
  #include <info.h>
  #include <PowerOnDetector.h>
  #include <rgbPanel.h>
  #include <CharGraphTimeLogic.h>
#endif
