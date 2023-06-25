// ----------------------
// PRE-REQUISITES
// === LoRa
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

int nodeIdSelf = 4;
 
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

String messageResponse = "Alert Received!";

int sess_start, sendTimeoutStart;

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

void listen_lora(int listenTimeout) {
  uint8_t len = sizeof(buf);
  uint8_t nodeIdFrom;

  if (listenTimeout == 0) {
    // continuous listening mode
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
  } else {
    // timeout listening mode
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
}

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    setup_lora();
    sess_start = millis();
    sendTimeoutStart = millis();
}

void loop()
{
  listen_lora(3000);
}
