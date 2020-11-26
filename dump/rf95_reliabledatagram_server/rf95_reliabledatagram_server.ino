// ----------------------
// PRE-REQUISITES
// === LoRa
#include <SPI.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1 // ESP32 max 512, Arduino Uno max 1024

/*// Arduino
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2 //*/

// ESP32
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2 //*/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

int nodeIdSelf = 2;
 
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
// RHMesh *manager;
RHReliableDatagram *manager;

String messageResponse = "DEFAULT_RESPONSE";

// ----------------------
// SETUP
// === LoRa
void setup_lora() {
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
  driver.setCADTimeout(500);

  // EEPROM.begin(EEPROM_SIZE); // gunakan untuk ESP32
  // nodeIdSelf = EEPROM.read(0);

  manager = new RHReliableDatagram(driver, nodeIdSelf);
  if (!manager->init()) {
      Serial.println(F("init failed"));
  } else {
      Serial.println("Mesh Node \"" + (String) nodeIdSelf + "\" Up and Running!");
  }
}

// ----------------------
// ACTIONS
// === LoRa
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];

void listen_lora() {
  uint8_t len = sizeof(buf);
  uint8_t nodeIdFrom;

  if (manager->recvfromAck(buf, &len, &nodeIdFrom))
  {
    Serial.print("Got Message nodeIdFrom ");
    Serial.print(nodeIdFrom, HEX);
    Serial.println(":");
    Serial.println((char*)buf);
    Serial.print("RSSI: ");
    Serial.println(driver.lastRssi(), DEC);

    Serial.println("lastRssi = " + (String) driver.lastRssi());

    // Reply with the same messageSent
    // ...write something Roy
    
    // Send a reply back to the originator client
    char messageResponseChar[messageResponse.length() + 1];
    strcpy(messageResponseChar, messageResponse.c_str());
    Serial.print("Sending Reply: ");
    Serial.println(messageResponseChar);
    if (manager->sendtoWait((uint8_t*) messageResponseChar, sizeof(messageResponseChar), nodeIdFrom) != RH_ROUTER_ERROR_NONE)
    Serial.println("sendtoWait failed");
  }
}

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    setup_lora();
    Serial.println("=== ReliableDatagram Server: Start ===");
}

void loop()
{
  listen_lora();
}
