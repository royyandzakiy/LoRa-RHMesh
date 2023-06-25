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

// Mesh
// In this small artifical network of 4 nodes,
#define CLIENT_ADDRESS 1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4

#ifndef NODE_ID
  #define NODE_ID CLIENT_ADDRESS
#endif // NODE_ID
#define NODE_ID_TARGET SERVER2_ADDRESS

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh loraMeshManager(rf95, NODE_ID);

uint8_t loraMsg[] = "Hello World!";
// Dont put this on the stack:
uint8_t loraMsgBuf[RH_MESH_MAX_MESSAGE_LEN]; // around 220 bytes/characters is safe

void setup() {
    Serial.begin(115200);
    
    if (!loraMeshManager.init()) {
        Serial.println(F("init failed"));
    } else {
        Serial.println("done");
    }
    
    rf95.setTxPower(23, false);
    rf95.setFrequency(RF95_FREQ);
    rf95.setCADTimeout(500);
}

void loop() 
{
  Serial.println("Sending to loraMeshManager_mesh_server2");
    
  // Send a message to a rf95_mesh_server
  // A route to the destination will be automatically discovered.
  if (loraMeshManager.sendtoWait(loraMsg, sizeof(loraMsg), NODE_ID_TARGET) == RH_ROUTER_ERROR_NONE)
  {
    // It has been reliably delivered to the next node.
    // Now wait for a reply from the ultimate server
    uint8_t loraMsgBufSize = sizeof(loraMsgBuf);
    uint8_t nodeIdFrom;    
    if (loraMeshManager.recvfromAckTimeout(loraMsgBuf, &loraMsgBufSize, 3000, &nodeIdFrom))
    {
      Serial.print("got reply from : 0x");
      Serial.print(nodeIdFrom, HEX);
      Serial.print(": ");
      Serial.println((char*)loraMsgBuf);
    }
    else
    {
      Serial.println("No reply, is rf95_mesh_server1, rf95_mesh_server2 and rf95_mesh_server3 running?");
    }
  }
  else
     Serial.println("sendtoWait failed. Are the intermediate mesh servers running?");
}
