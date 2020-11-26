// Arduino9x_RX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Arduino9x_TX
 
#include <SPI.h>
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
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
int count = 0;

int pick_config = 4;

void reconfig(int config_num) {
  Serial.println("Modem Configuration: " + (String) config_num);
  if (config_num == 1) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr45Sf128 (default)
      0x72, // Reg 0x1D: RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    rf95.setModemRegisters(&modem_config);
  } else if (config_num == 2) {
    RH_RF95::ModemConfig modem_config = { // Bw500Cr45Sf128
      0x92, // Reg 0x1D: RH_RF95_BW_500KHZ | RH_RF95_CODING_RATE_4_5
      0x74, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_128CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: automatic AGC on
    };
    rf95.setModemRegisters(&modem_config);
  } else if (config_num == 3) {
    RH_RF95::ModemConfig modem_config = { // Bw31_25Cr48Sf512
      0x48, // Reg 0x1D: RH_RF95_BW_31_25KHZ | RH_RF95_CODING_RATE_4_8
      0x94, // Reg 0x1E: RH_RF95_SPREADING_FACTOR_512CPS | RH_RF95_PAYLOAD_CRC_ON
      0x04  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    }; 
    rf95.setModemRegisters(&modem_config);
  } else if (config_num == 4) {
    RH_RF95::ModemConfig modem_config = { // Bw125Cr48Sf4096
      0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      0xC4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      0x08  // Reg 0x26: 0x00 AGC off; 0x04 automatic AGC on; 0x08 LowDataRate=On, Agc=Off. 0x0C is LowDataRate=ON, ACG=ON
    };
    rf95.setModemRegisters(&modem_config);
  }
}
 
void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
 
  while (!Serial);
  Serial.begin(9600);
  delay(100);
 
  Serial.println("ESP32 LoRa RX Test!");
  
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
 
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false);
  rf95.setCADTimeout(500);

  reconfig(pick_config);

  Serial.println("RF95 ready");
}
 
void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      count++;
      Serial.print((String) count);
      Serial.print((char*)buf);
      Serial.print("; RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}
