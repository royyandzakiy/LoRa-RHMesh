#include <EEPROM.h>

#define EEPROM_SIZE 1 // ESP32 max 512, Arduino Uno max 1024

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  EEPROM.begin(EEPROM_SIZE); // gunakan untuk ESP32
  uint8_t readVal = EEPROM.read(0);

  Serial.print(F("read Value in EEPROM = "));
  Serial.println(readVal);
}

void loop() {

}
