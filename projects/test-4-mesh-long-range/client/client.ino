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

/*// TTGO T-BEAM
#define RFM95_CS 18
#define RFM95_RST 14
#define RFM95_INT 26 //*/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

int nodeIdSelf = 1;
int nodeIdDestination = 4;

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

int sess_start, sendTimeoutStart, delivCount;

String message = "Hello! from node " + (String) nodeIdSelf + ", to node " + (String) nodeIdDestination;
String messageResponse = "Alert Received!";

// ----------------------
// SETUP
// === LoRa
int pick_config = 4;

void reconfig(int config_num) {
  Serial.println("Modem Configuration: " + (String) config_num);
  if (config_num == 1) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr45Sf128 (default)
      0x72, // Reg 0x1D: RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    driver.setModemRegisters(&modem_config);
  } else if (config_num == 2) {
    RH_RF95::ModemConfig modem_config = { // Bw500Cr45Sf128
      0x92, // Reg 0x1D: RH_RF95_BW_500KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    };
    driver.setModemRegisters(&modem_config);
  } else if (config_num == 3) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x48, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x94, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    driver.setModemRegisters(&modem_config);
  } else if (config_num == 4) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr48Sf4096
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    };
    driver.setModemRegisters(&modem_config);
  }
}

void setup_lora() {  
  pinMode(RFM95_RST, OUTPUT);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  
  while (!driver.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  if (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  manager = new RHMesh(driver, nodeIdSelf);
  
  if (!manager->init()) {
      Serial.println(F("init failed"));
  } else {
      Serial.println("Mesh Node \"" + (String) nodeIdSelf + "\" Up and Running!");
  }
  
  driver.setTxPower(23, false);
  driver.setCADTimeout(500);
  reconfig(pick_config);
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

    int send_start = millis();
    int errorLog = manager->sendtoWait((uint8_t*) messageChar, sizeof(messageChar), nodeIdDestination);
    if (errorLog == RH_ROUTER_ERROR_NONE)
    {
      // It has been reliably delivered to the next node.
      // count how long sending take
      int send_end = millis();
      int send_time = send_end - send_start;
      Serial.println("sending time: " + (String) send_time);

      // Now wait for a reply from the ultimate server      
      uint8_t len = sizeof(buf);
      uint8_t from;    
      if (manager->recvfromAckTimeout(buf, &len, 20000, &from))
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

void listen_lora(int listenTimeout) {
  uint8_t len = sizeof(buf);
  uint8_t nodeIdFrom;

  if (manager->recvfromAckTimeout(buf, &len, listenTimeout, &nodeIdFrom))
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

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    setup_lora();
    delivCount = 0;
    sess_start = millis();
    sendTimeoutStart = millis();
}

void loop() 
{
  int sendInterval = 2000;
  int sendSimultCount = 1;
  if ((millis() - sendTimeoutStart) > sendInterval) {
    sendTimeoutStart = millis();
    sendMessage(message, nodeIdDestination, sendSimultCount);
    delivCount++;
    Serial.println((String) delivCount + "/200 sent");
  }
  
  // listen_lora(3000);  
  
  if (delivCount >= 200) {
    int sess_end = millis();
    int sess_time = sess_end - sess_start;
    Serial.println("Total time: " + (String) sess_time);
    while(1);
  }
}
