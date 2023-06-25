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

int nodeIdSelf;
int nodeIdDestination = 2;
 
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

String message = "ALERT::NODE 3::117.2,50.1::Msg:!!!";

// === Trigger
#define PUSH_BUTTON 13

// ----------------------
// SETUP
// === LoRa
void setup_lora() {
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
  driver.setCADTimeout(500);

  EEPROM.begin(EEPROM_SIZE); // gunakan untuk ESP32
  nodeIdSelf = EEPROM.read(0);

  manager = new RHMesh(driver, nodeIdSelf);
  
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

void sendMessage(String _message, int _nodeIdDestination, int _sendMessageCount) {
  for (int i=0; i<_sendMessageCount; i++) {
    Serial.println("Sending to RF95 Mesh Node \"" + (String) _nodeIdDestination + "\"!");
    // Send a message to a rf95_mesh_node
    // A route to the destination will be automatically discovered.

    char messageChar[_message.length() + 1];
    strcpy(messageChar, _message.c_str());
    
    int errorLog = manager->sendtoWait((uint8_t*) messageChar, sizeof((uint8_t*) messageChar), nodeIdDestination);
    if (errorLog == RH_ROUTER_ERROR_NONE)
    {
      // It has been reliably delivered to the next node.
      // Now wait for a reply from the ultimate server
      uint8_t len = sizeof(buf);
      uint8_t from;    
      if (manager->recvfromAckTimeout(buf, &len, 3000, &from))
      {
        Serial.print("Got Reply from ");
        Serial.print(from, HEX);
        Serial.println(":");
        Serial.println((char*)buf);}
      else
      {
        Serial.println("No reply, is rf95_mesh_node1, rf95_mesh_node2 and rf95_mesh_node3 running?");
      }
    }
    else
       Serial.println("sendtoWait failed. Are the intermediate mesh nodes running?");
  }
}

// === Trigger
int startTime = 0;
boolean onTrigger = false;

void triggerMechanism(int triggerInterval) {
  // onTrigger true dipicu oleh menekan PUSH_BUTTON
  // masa trigger akan berlangsung selama interval waktu tertentu yang ditentukan triggerInterval
  // selama masa interval, akan dilakukan pengiriman secara berkala sesuai interval
  // setelah interval habis, maka onTrigger akan false

  // wait for button press, then trigger if button pressed
  if (digitalRead(PUSH_BUTTON) == HIGH) {
    if (!onTrigger) {
      onTrigger = true;
      startTime = millis(); // catat waktu mulai

      Serial.println("TRIGGERED");
      sendMessage(message, nodeIdDestination, 5); // kirim pesan sebagai simulasi alert
    }
  }

  // check if triggerInterval has ran up
  if (onTrigger) {
    int count = millis() - startTime; // count how long has the time passed
    if (count > triggerInterval) {
      // time is up, reset onTrigger to false
      onTrigger = false; // reset
      Serial.println("Trigger ready...");
    }
  }
}

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    setup_lora();
}

void loop() 
{
  triggerMechanism(5000); // interval until next button press is 5000ms
}
