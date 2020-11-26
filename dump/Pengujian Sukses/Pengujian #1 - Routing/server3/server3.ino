// END NODE - SERVER 3 (S3)
// TOPOLOGY:
// C - S1 - S3 (C and S3 has no direct connection/route)

// rf95_router_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging client
// with the RHRouter class.
// It is designed to work with the other examples rf95_router_server*

#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

// In this small artifical network of 4 nodes,
// messages are routed via intermediate nodes to their destination
// node. All nodes can act as routers
// CLIENT_ADDRESS <-> SERVER1_ADDRESS <-> SERVER2_ADDRESS<->SERVER3_ADDRESS
#define CLIENT_ADDRESS 1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4

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
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHRouter *manager;
int count = 0;

void setup() 
{
  Serial.begin(9600);
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
  driver.setCADTimeout(500);
  
  manager = new RHRouter(driver, SERVER3_ADDRESS);
  
  if (!manager->init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  
  // Manually define the routes for this network
//  manager->addRouteTo(CLIENT_ADDRESS, CLIENT_ADDRESS);

  manager->addRouteTo(CLIENT_ADDRESS, SERVER1_ADDRESS);
  manager->addRouteTo(SERVER1_ADDRESS, SERVER1_ADDRESS);    
//  manager->addRouteTo(SERVER2_ADDRESS, SERVER2_ADDRESS);
//  manager->addRouteTo(SERVER3_ADDRESS, SERVER2_ADDRESS);


  Serial.println("Router Server " + (String) (SERVER3_ADDRESS-1) + ": Up and Running");
}

uint8_t data[] = "And hello back to you from server3";
// Dont put this on the stack:
uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager->recvfromAck(buf, &len, &from))
  {
    Serial.print("got request from : 0x");
    Serial.print(from, HEX);
    Serial.print(": ");
    Serial.println((char*)buf);
    count++;
    Serial.println("Count Recieved: " + (String) count + "/100");

    // Send a reply back to the originator client
    if (manager->sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
      Serial.println("sendtoWait failed");
  }
}
