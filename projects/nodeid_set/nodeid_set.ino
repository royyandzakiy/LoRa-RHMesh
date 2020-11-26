#include <EEPROM.h>

#define EEPROM_SIZE 1 // ESP32 max 512, Arduino max 1024
// change this to be the ID of your node in the mesh network
uint8_t nodeId = 1;

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  Serial.println("setting nodeId...");
  
  EEPROM.begin(EEPROM_SIZE); // gunakan untuk ESP32/
  EEPROM.write(0, nodeId);
  EEPROM.commit(); // gunakan untuk ESP32/

  // BONUS
  // EEPROM.update(0, nodeIdBaru); // bonus kali aja butuh
  // nodeId = EEPROM.read(0); // bonus kali aja butuh
  
  Serial.print(F("set nodeId = "));
  Serial.println(nodeId);

  uint8_t readVal = EEPROM.read(0);

  Serial.print(F("read nodeId = "));
  Serial.println(readVal);

  if (nodeId != readVal) {
    Serial.println(F("*** FAIL ***"));
  } else {
    Serial.println(F("SUCCESS"));
  }
}

void loop() {

}
