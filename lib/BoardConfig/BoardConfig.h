#pragma once

// Radio pins and frequency shared by src/ and examples/.
// platformio.ini sets the pins per board, the defaults are the ESP32 DOIT devkit wiring.

#include <Arduino.h>

#ifndef RFM95_CS
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2
#endif

// Must match every other node and the module's band (433, 868 or 915 MHz).
#ifndef RF95_FREQ
#define RF95_FREQ 915.0
#endif

// Hardware reset so the modem starts clean after an ESP32-only reboot (e.g. watchdog).
inline void resetRadio() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}
