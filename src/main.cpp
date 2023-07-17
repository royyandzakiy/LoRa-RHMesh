// #define RH_TEST_NETWORK 1 // activate Forced Topology

#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>

#include <cstring>

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2

#define RF95_FREQ 915.0
#define RH_MESH_MAX_MESSAGE_LEN 100

#define SENDING_MODE 0
#define RECEIVING_MODE 1

// Topology
#define NODE1_ADDRESS 1
#define NODE2_ADDRESS 2
#define NODE3_ADDRESS 3
#define FINAL_ADDRESS 255  // purposefully using the last namber

#if defined(SELF_ADDRESS) && defined(TARGET_ADDRESS)
const uint8_t selfAddress_ = SELF_ADDRESS;
const uint8_t targetAddress_ = TARGET_ADDRESS;
#else
const uint8_t selfAddress_ = NODE3_ADDRESS;  // CHANGE THIS!!!
const uint8_t targetAddress_ = FINAL_ADDRESS;
#endif

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
  Serial.println(" ---------------- LORA NODE " + String(selfAddress_) +
                 " INIT ---------------- ");
}

long _lastSend = 0, sendInterval_ = 3000;  // send every 10 seconds
uint8_t _msgRcvBuf[RH_MESH_MAX_MESSAGE_LEN];

void loop() {
  uint8_t _msgFrom;
  uint8_t _msgRcvBufLen = sizeof(_msgRcvBuf);

  if ((millis() - _lastSend > sendInterval_) && selfAddress_ != FINAL_ADDRESS) {
    mode_ = SENDING_MODE;
  }

  if (mode_ == SENDING_MODE) {
    // Send a message to another rhmesh node
    Serial.printf("Sending to %d...", targetAddress_);
    uint8_t _err =
        RHMeshManager_.sendtoWait(reinterpret_cast<uint8_t *>(&msgSend[0]),
                                  msgSend.size(), targetAddress_);
    if (_err == RH_ROUTER_ERROR_NONE) {
      // message successfully be sent to the target node, or next neighboring
      // expecting to recieve a simple reply from the target node
      Serial.printf(" successfull! Awaiting for Reply\n");

      if (RHMeshManager_.recvfromAckTimeout(_msgRcvBuf, &_msgRcvBufLen, 3000,
                                            &_msgFrom)) {
        Serial.printf("Received a Reply: [%d] \"%s\"\n",
        _msgFrom, reinterpret_cast<char *>(_msgRcvBuf));
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
    Serial.println("Receiving mode active");

    if (RHMeshManager_.recvfromAckTimeout(_msgRcvBuf, &_msgRcvBufLen, 3000,
                                          &_msgFrom)) {
      Serial.println("Received a message");
      char buf_[RH_MESH_MAX_MESSAGE_LEN];
      std::sprintf(buf_, "%s", reinterpret_cast<char *>(_msgRcvBuf));
      msgRcv = std::string(buf_);

      // do something with message, for example pass it through a callback
      Serial.printf("[%d] \"%s\". Sending a reply...\n", _msgFrom,
                    msgRcv.c_str());

      msgRcv = "";

      std::string _msgRply("Hi node %d, got the message!", _msgFrom);
      uint8_t _err = RHMeshManager_.sendtoWait(
          reinterpret_cast<uint8_t *>(&_msgRply[0]), _msgRply.size(), _msgFrom);
      if (_err == RH_ROUTER_ERROR_NONE) {
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