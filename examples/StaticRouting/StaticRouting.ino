// Static routing with RHRouter: routes are written by hand instead of discovered.
// Line topology 1 - 2 - 3. Node 1 sends to node 3 through node 2, node 3 replies.
// Compare with src/main.cpp, where RHMesh finds the same route on its own.
//
// On a desk every node hears every other, so the static-routing envs in platformio.ini
// also set RH_TEST_NETWORK=4, which makes RHRouter drop frames that skip node 2.

#include <LoRaRHMesh.h>
#include <RHRouter.h>
#include <RH_RF95.h>
#include <SPI.h>

// Change for every board: 1, 2 or 3.
#ifndef SELF_ADDRESS
#define SELF_ADDRESS 1
#endif

const uint8_t selfAddress_ = SELF_ADDRESS;
const unsigned long sendIntervalMs_ = 3000;

RH_RF95 rfm95Modem_(RFM95_CS, RFM95_INT);
RHRouter router_(rfm95Modem_, selfAddress_);

// Only the next hop is stored: node 1 knows 3 is "via 2", not the full path.
void addStaticRoutes() {
  switch (selfAddress_) {
    case 1:
      router_.addRouteTo(2, 2);
      router_.addRouteTo(3, 2);
      break;
    case 2:
      router_.addRouteTo(1, 1);
      router_.addRouteTo(3, 3);
      break;
    case 3:
      router_.addRouteTo(2, 2);
      router_.addRouteTo(1, 2);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  rhmesh::resetRadio();

  if (!router_.init()) Serial.println("init failed");
  rfm95Modem_.setFrequency(RF95_FREQ);
  rfm95Modem_.setTxPower(23, false);
  rfm95Modem_.setCADTimeout(500);

  addStaticRoutes();
  Serial.printf("Static router node %d, routing table:\n", selfAddress_);
  router_.printRoutingTable();
}

void sendTo(uint8_t dest, const char* text) {
  const uint8_t err = router_.sendtoWait(reinterpret_cast<uint8_t*>(const_cast<char*>(text)), strlen(text), dest);
  // NONE only means the next hop ACKed, not that dest received it.
  Serial.printf("send to %d: %s\n", dest, err == RH_ROUTER_ERROR_NONE ? "next hop ACKed" : "failed");
}

void loop() {
  static unsigned long lastSend = 0;
  static uint32_t counter = 0;

  if (selfAddress_ == 1 && millis() - lastSend > sendIntervalMs_) {
    char msg[32];
    snprintf(msg, sizeof(msg), "ping #%lu", static_cast<unsigned long>(counter++));
    sendTo(3, msg);
    lastSend = millis();
  }

  // Every node must keep receiving, that is also when node 2 forwards.
  uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  uint8_t from, hops;
  if (router_.recvfromAckTimeout(buf, &len, 200, &from, nullptr, nullptr, nullptr, &hops)) {
    Serial.printf("[%d] \"%.*s\" rssi %d, %d hop(s)\n", from, len, reinterpret_cast<char*>(buf),
                  rfm95Modem_.lastRssi(), hops);
    if (selfAddress_ == 3) sendTo(from, "pong");
  }
}
