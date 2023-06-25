#include <SPI.h>
#include <RHMesh.h>
#include <RH_RF95.h>

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

int nodeIdSelf = 1;
int nodeIdDestination = 4;
int startTimer;
int sendInterval = 2000;

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

String message;
String messageResponse = "Alert Received!";

int count = 0;

// ----------------------
// SETUP
// === LoRa
void setup_lora() {  
  manager = new RHMesh(driver, nodeIdSelf);
  
  if (!manager->init()) {
      Serial.println(F("init failed"));
  } else {
      Serial.println("Mesh Node \"" + (String) nodeIdSelf + "\" Up and Running!");
  }
  
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
  driver.setCADTimeout(500);

  // long range configuration requires for on-air time
  boolean longRange = true;
  if (longRange) {
    RH_RF95::ModemConfig modem_config = {
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: LowDataRate=On, Agc=Off.  0x0C is LowDataRate=ON, ACG=ON
    };
    driver.setModemRegisters(&modem_config);
    if (!driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
      Serial.println(F("set config failed"));
    }
  }

  Serial.println("RF95 ready");
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
    
    int errorLog = manager->sendtoWait((uint8_t*) messageChar, sizeof(messageChar), nodeIdDestination);
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

void listen_lora() {
  uint8_t len = sizeof(buf);
  uint8_t nodeIdFrom;

  if (manager->recvfromAck(buf, &len, &nodeIdFrom))
  {
    Serial.print("Got Message nodeIdFrom ");
    Serial.print(nodeIdFrom, HEX);
    Serial.println(":");
    Serial.println((char*)buf);

    Serial.println("lastRssi = " + (String) driver.lastRssi());
    
    // Send a reply back to the originator client
    char messageResponseChar[messageResponse.length() + 1];
    strcpy(messageResponseChar, messageResponse.c_str());
    if (manager->sendtoWait((uint8_t*) messageResponseChar, sizeof(messageResponseChar), nodeIdFrom) != RH_ROUTER_ERROR_NONE)
      Serial.println("sendtoWait failed");
  }
}

void setup() {
    Serial.begin(9600);
    setup_lora();
    startTimer = millis();
    message = "Hello! from node " + (String) nodeIdSelf + ", to node " + (String) nodeIdDestination;
}

void loop() 
{
  if (millis() - startTimer > sendInterval) {
    startTimer = millis();
    sendMessage(message, nodeIdDestination, 1);
    count++;
    Serial.println((String) count + "/200 sent");
  }
  listen_lora();  
  if (count >= 200) while(1);
}
