// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX
 
#include <SPI.h>
#include <RH_RF95.h>
  
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
// #define RF95_FREQ 868.0
// #define RF95_FREQ 433.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
   
  Serial.begin(9600);
//  while (!Serial);
//  delay(100);
 
  Serial.println("ESP32 LoRa TX Test!");
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  rf95.setCADTimeout(500);

  int pick_config = 4;
  if (pick_config == 1) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr45Sf128 (default)
      0x72, // Reg 0x1D: RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    }; 
    rf95.setModemRegisters(&modem_config);
  } else if (pick_config == 2) {
    RH_RF95::ModemConfig modem_config = { // Bw500Cr45Sf128
      0x92, // Reg 0x1D: RH_RF95_BW_500KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    };
    rf95.setModemRegisters(&modem_config);
  } else if (pick_config == 3) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x48, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x94, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    rf95.setModemRegisters(&modem_config);
  } else if (pick_config == 4) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr48Sf4096
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    };
    rf95.setModemRegisters(&modem_config);
  } else if (pick_config == 5) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x02, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x60, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x00  // 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    rf95.setModemRegisters(&modem_config);
  } else if (pick_config == 6) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x12, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0xc4, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    rf95.setModemRegisters(&modem_config);
  }
  
//  boolean longRange = true;
//  if (longRange) {
//    RH_RF95::ModemConfig modem_config = {
//      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
//      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
//      0x08  // Reg 0x26: LowDataRate=On, Agc=Off.  0x0C is LowDataRate=ON, ACG=ON
//    };
//    rf95.setModemRegisters(&modem_config);
//    if (!rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096)) {
//      Serial.println(F("set config failed"));
//    }
//  }

  Serial.println("RF95 ready");
}
 
int16_t packetnum = 0;  // packet counter, we increment per xmission

String message = "Hello World LoRa #";

void sendMessage(String _message, int _nodeIdDestination, int _sendMessageCount) {
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server
  _message = _message + String(packetnum++);
  
  char radiopacket[_message.length() + 1];
  strcpy(radiopacket, _message.c_str());
  Serial.print("Sending "); Serial.println(radiopacket);
  
  Serial.println("Sending..."); delay(10);
  Serial.print("Send Success: ");
  Serial.println((String) rf95.send((uint8_t *)radiopacket, 20));
 
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
 
  Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }  
}

void loop()
{
  sendMessage(message, 0, 1);
  delay(1000);
}
