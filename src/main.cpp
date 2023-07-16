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

#define RF95_FREQ 915.0
#define RH_MESH_MAX_MESSAGE_LEN 50

#define SENDING_MODE 0
#define RECEIVING_MODE 1

// Topology
#define NODE1_ADDRESS 1
#define NODE2_ADDRESS 2
#define NODE3_ADDRESS 3
#define FINAL_ADDRESS 254  // purposefully using the last namber

const uint8_t selfAddress_ = NODE3_ADDRESS;  // CHANGE THIS!!!
const uint8_t targetAddress_ = FINAL_ADDRESS;

// radio driver & message mesh delivery/receipt manager
RH_RF95 RFM95Modem_(RFM95_CS, RFM95_INT);
RHMesh RHMeshManager_(RFM95Modem_, selfAddress_);
uint8_t mode_ = RECEIVING_MODE;

// these are expected to be global/externally exposed variables, if you plan to
// make a class to wrap this
std::string msgSend = "Hello, from " + selfAddress_;
std::string msgRcv;

void rhSetup();

void setup() {
  Serial.begin(115200);
  rhSetup();
}

long _lastSend = 0, sendInterval_ = 10000;  // send every 10 seconds

void loop() {
  uint8_t _msgFrom;
  uint8_t _msgRcvBuf[RH_MESH_MAX_MESSAGE_LEN];

  if ((millis() - _lastSend > sendInterval_) && selfAddress_ != FINAL_ADDRESS) {
    mode_ = SENDING_MODE;
  }

  if (mode_ == SENDING_MODE) {
    // Send a message to another rhmesh node
    Serial.println("Sending to " + targetAddress_);
    if (RHMeshManager_.sendtoWait(reinterpret_cast<uint8_t *>(&msgSend[0]),
                                  msgSend.size(),
                                  targetAddress_) == RH_ROUTER_ERROR_NONE) {
      // message successfully be sent to the target node, or next neighboring
      // expecting to recieve a simple reply from the target node
      if (RHMeshManager_.recvfromAckTimeout(
              _msgRcvBuf, (uint8_t *)sizeof(_msgRcvBuf), 3000, &_msgFrom)) {
        msgRcv = std::string("got reply from : " + std::to_string(_msgFrom) +
                             ":" + reinterpret_cast<char *>(_msgRcvBuf));
        Serial.printf("%s\n", msgRcv);
      } else {
        Serial.println("No reply, is the target node running?");
      }
    } else {
      Serial.println(
          "sendtoWait failed. No response from intermediary node, are they "
          "running?");
    }
    _lastSend = millis();
    mode_ = RECEIVING_MODE;
  }

  if (mode_ == RECEIVING_MODE) {
    // while at it, wait for a message from other nodes
    if (RHMeshManager_.recvfromAckTimeout(
            _msgRcvBuf, (uint8_t *)sizeof(_msgRcvBuf), 3000, &_msgFrom)) {
      msgRcv = std::string("got msg from : " + std::to_string(_msgFrom) + ":" +
                           reinterpret_cast<char *>(_msgRcvBuf) +
                           ". sending a reply...");
      Serial.printf("%s\n", msgRcv);

      std::string _msgRply("Hi node %d, got the message!", _msgFrom);

      if (RHMeshManager_.sendtoWait(reinterpret_cast<uint8_t *>(&_msgRply[0]),
                                    _msgRply.size(),
                                    _msgFrom) == RH_ROUTER_ERROR_NONE) {
        // message successfully received by either final target node, or next
        // neighboring node. do nothing...
      } else {
        Serial.println("No messages coming in...");
      }
    }
  }
}

void rhSetup() {
  if (!RHMeshManager_.init()) Serial.println("init failed");
  RFM95Modem_.setTxPower(23, false);
  RFM95Modem_.setFrequency(RF95_FREQ);
  RFM95Modem_.setCADTimeout(500);
}