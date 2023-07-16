// Example sketch showing how to create a simple addressed, routed reliable
// messaging client with the RHMesh class. It is designed to work with the other
// examples rf22_mesh_server* Hint: you can simulate other network topologies by
// setting the RH_TEST_NETWORK define in RHRouter.h

#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <cstring>

// pinout
// [RFM95] ------------- [ESP32]
// RESET  -------------- GPIO14
// NSS/CS -------------- GPIO5
// SCK    -------------- GPIO18
// MOSI   -------------- GPIO23
// MISO   -------------- GPIO19
// DIO0   -------------- GPIO2
// 
// 3.3V   -------------- 3.3V
// GND    -------------- GND

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Mesh has much greater memory requirements, and you may need to limit the
// max message length (characters) to prevent wierd crashes
#define RH_MESH_MAX_MESSAGE_LEN 50

// This example of 4 node topology, in which the FINAL_ADDRESS node is
// expected to be the last node, simulating a common network in which this
// last node would be a border node connected to the internet, collecting
// messages from other nodes during it's lifetime. whilst node 1-3 would
// commonly send sensor data, and one could be an intermediary node for
// another
#define NODE1_ADDRESS 1
#define NODE2_ADDRESS 2
#define NODE3_ADDRESS 3
#define FINAL_ADDRESS 4

// change this!
#define SELF_ADDRESS NODE3_ADDRESS
#define TARGET_ADDRESS FINAL_ADDRESS

// radio driver & message mesh delivery/receipt manager
RH_RF95 RHDriver(RFM95_CS, RFM95_INT);
RHMesh RHMeshManager(RHDriver, SELF_ADDRESS);

std::string msgSend = "Hello, from " + SELF_ADDRESS;
std::string msgRcv;

void rhSetup();

void setup() {
  Serial.begin(115200);
  rhSetup();
}

void loop() {
  Serial.println("Sending to " + TARGET_ADDRESS);

  // Send a message to another rhmesh node
  // A route to the destination will be automatically discovered.
  if (RHMeshManager.sendtoWait(reinterpret_cast<uint8_t *>(&msgSend[0]),
                               msgSend.size(),
                               TARGET_ADDRESS) == RH_ROUTER_ERROR_NONE) {
    // It has been reliably delivered to the next node.
    // Now wait for a reply from the target node
    uint8_t _msgFrom;
    uint8_t _msgRcvBuf[RH_MESH_MAX_MESSAGE_LEN];
    if (RHMeshManager.recvfromAckTimeout(_msgRcvBuf, (uint8_t *) sizeof(_msgRcvBuf), 3000, &_msgFrom)) {
      msgRcv = std::string("got reply from : " + std::to_string(_msgFrom) + ":" + reinterpret_cast<char*>(_msgRcvBuf));
    } else {
      Serial.println("No reply, are the other nodes running?");
    }
  } else
    Serial.println(
        "sendtoWait failed. Are the other nodes running?");
}

void rhSetup() {
  if (!RHMeshManager.init()) Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation
  // FSK_Rb2_4Fd36. change to this
  RHDriver.setTxPower(23, false);
  RHDriver.setFrequency(RF95_FREQ);
  RHDriver.setCADTimeout(500);
}