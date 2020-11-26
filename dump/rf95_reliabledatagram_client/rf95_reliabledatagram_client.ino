// ----------------------
// PRE-REQUISITES
// === LoRa
#include <SPI.h>
#include <RHMesh.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1 // ESP32 max 512, Arduino Uno max 1024

/*// Arduino
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2 //*/

/*// ESP32
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2 //*/

// TTGO LoRa32 SX1276
#define RFM95_CS 18
#define RFM95_RST 14
#define RFM95_INT 26 //*/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

int nodeIdSelf = 1;
int nodeIdDestination = 2;
 
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
// RHMesh *manager;
RHReliableDatagram *manager;

#define MAX_PACKET_NUMBER 100
int packet_number = 1;
String timestamp = "\"hh:mm:ss\"";
// String message = "{\"packet\":" + (String) packet_number + ", \"timestamp\":" + timestamp + "}";
String message = "DEFAULT_MESSAGE";

// === Trigger
#define PUSH_BUTTON 13

// ----------------------
// SETUP
// === LoRa
void setup_lora() {
  
  pinMode(RFM95_RST, OUTPUT);
  delay(100);
 
  Serial.println("ESP32 LoRa TX Test!");
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
   
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
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  
  driver.setTxPower(23, false);
  driver.setCADTimeout(500);

  int pick_config = 4;
  if (pick_config == 1) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr45Sf128 (default)
      0x72, // Reg 0x1D: RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    }; 
    driver.setModemRegisters(&modem_config);
  } else if (pick_config == 2) {
    RH_RF95::ModemConfig modem_config = { // Bw500Cr45Sf128
      0x92, // Reg 0x1D: RH_RF95_BW_500KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    };
    driver.setModemRegisters(&modem_config);
  } else if (pick_config == 3) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x48, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x94, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    driver.setModemRegisters(&modem_config);
  } else if (pick_config == 4) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr48Sf4096
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    };
    driver.setModemRegisters(&modem_config);
  } else if (pick_config == 5) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x02, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x60, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x00  // 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    driver.setModemRegisters(&modem_config);
  } else if (pick_config == 6) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x12, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0xc4, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    driver.setModemRegisters(&modem_config);
  }
  
//  boolean longRange = true;
//  if (longRange) {
//    RH_RF95::ModemConfig modem_config = {
//      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
//      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
//      0x08  // Reg 0x26: LowDataRate=On, Agc=Off.  0x0C is LowDataRate=ON, ACG=ON
//    };
//    driver.setModemRegisters(&modem_config);
//    if (!rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
//      Serial.println(F("set config failed"));
//    }
//  }

  Serial.println("RF95 ready");

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
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(driver.lastRssi(), DEC);
    } else
      {
        Serial.println("No reply, is rf95_mesh_node1, rf95_mesh_node2 and rf95_mesh_node3 running?");
      }
    }
    else
       Serial.println("sendtoWait failed. Are the intermediate mesh nodes running?");
    packet_number++; // increment packet_number
  }
}


// === Trigger
int startTime = 0;
boolean onTrigger = false;

void sendMechanism(int sendInterval) {
  int count = millis() - startTime; // count how long has the time passed
  if (count > sendInterval) {
    // send, reset startTime
    Serial.print("Send Message: ");
    Serial.println(message);
    sendMessage(message, nodeIdDestination, 1); // kirim pesan sebagai simulasi alert
    startTime = millis(); // catat waktu mulai
  }
}

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    while (!Serial);
    delay(100);
   
    Serial.println("ESP32 LoRa TX Test!");
    Serial.println("=== ReliableDatagram Client: Setup ===");
    setup_lora();
    Serial.println("=== ReliableDatagram Client: Start ===");
}

void loop() 
{
  sendMechanism(2000); // interval until next button press is 5000ms
  if (packet_number >= MAX_PACKET_NUMBER) while(1); // stop sending
}
