// Forced topology: pass -D RH_TEST_NETWORK=N as a build flag (see platformio.ini).
// A #define here has no effect, RHRouter.cpp is compiled separately.

#include <BoardConfig.h>
#include <MeshNode.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <esp_task_wdt.h>

// Timing defaults suit the default modem setting (Bw125Cr45Sf128, ~60 ms per frame).
// Slower settings need all of them raised, see the longrange envs in platformio.ini.
#ifndef SEND_INTERVAL_MS
#define SEND_INTERVAL_MS 3000
#endif
// Per-hop ACK wait, RadioHead's default. Must cover a frame and its ACK on air.
#ifndef HOP_TIMEOUT_MS
#define HOP_TIMEOUT_MS 200
#endif
// End-to-end ACK wait, must cover the trip there and back over every hop.
#ifndef ACK_TIMEOUT_MS
#define ACK_TIMEOUT_MS 3000
#endif
// Longer than the slowest send (route discovery plus ACK timeout), or the watchdog resets mid-send.
#ifndef WDT_TIMEOUT_S
#define WDT_TIMEOUT_S 15
#endif

// The end node only listens and ACKs. Not 255, that is RadioHead's broadcast address.
#ifndef ENDNODE_ADDRESS
#define ENDNODE_ADDRESS 254
#endif

#if defined(SELF_ADDRESS) && defined(TARGET_ADDRESS)
const uint8_t selfAddress_ = SELF_ADDRESS;
const uint8_t targetAddress_ = TARGET_ADDRESS;
#else
const uint8_t selfAddress_ = 3;  // CHANGE THIS for every node
const uint8_t targetAddress_ = ENDNODE_ADDRESS;
#endif

static_assert(selfAddress_ <= MeshNode::kMaxNodeAddress, "255 is the broadcast address");
static_assert(targetAddress_ <= MeshNode::kMaxNodeAddress, "255 is the broadcast address");

const unsigned long sendIntervalMs_ = SEND_INTERVAL_MS;

RH_RF95 rfm95Modem_(RFM95_CS, RFM95_INT);
MeshNode meshNode_(rfm95Modem_, selfAddress_);

void rhSetup();
void onMessage(const MeshNode::Message& msg, void*);
void sendHello();

void setup() {
  Serial.begin(115200);
  esp_task_wdt_init(WDT_TIMEOUT_S, true);  // panic on timeout so the ESP32 restarts
  esp_task_wdt_add(NULL);

  rhSetup();
  meshNode_.onMessage(onMessage);
  Serial.printf(" ---------------- LORA NODE %d INIT ---------------- \n", selfAddress_);
}

void loop() {
  static unsigned long lastSend = 0;

  // The end node only listens and ACKs, every other node sends to it periodically.
  if (selfAddress_ != ENDNODE_ADDRESS && millis() - lastSend > sendIntervalMs_) {
    sendHello();
    lastSend = millis();
  }

  meshNode_.poll(500);
  esp_task_wdt_reset();
}

void sendHello() {
  static uint32_t counter = 0;
  char msg[MeshNode::kMaxPayload];
  const int len = snprintf(msg, sizeof(msg), "Hello from node %d #%lu", selfAddress_,
                           static_cast<unsigned long>(counter++));

  Serial.printf("Sending \"%s\" to %d... ", msg, targetAddress_);
  const MeshNode::SendResult result =
      meshNode_.send(targetAddress_, reinterpret_cast<const uint8_t*>(msg), len);
  Serial.printf("%s", MeshNode::resultName(result));

  uint8_t nextHop;
  if (meshNode_.nextHopTo(targetAddress_, &nextHop)) {
    Serial.printf(" (next hop %d)", nextHop);
  }
  Serial.println();
  esp_task_wdt_reset();
}

void onMessage(const MeshNode::Message& msg, void*) {
  // Payload is not null-terminated, print it with an explicit length.
  Serial.printf("[%d] \"%.*s\" rssi %d, %d hop(s)\n", msg.from, msg.len,
                reinterpret_cast<const char*>(msg.data), msg.rssi, msg.hops);
}

void rhSetup() {
  resetRadio();

  if (!meshNode_.init()) Serial.println("init failed");
  rfm95Modem_.setTxPower(23, false);
  rfm95Modem_.setFrequency(RF95_FREQ);
  rfm95Modem_.setCADTimeout(500);
#ifdef MODEM_CONFIG
  // One of RH_RF95::ModemConfigChoice, e.g. -D MODEM_CONFIG=Bw125Cr48Sf4096.
  rfm95Modem_.setModemConfig(RH_RF95::MODEM_CONFIG);
#endif

  meshNode_.setHopTimeout(HOP_TIMEOUT_MS);
  meshNode_.setAckTimeout(ACK_TIMEOUT_MS);
}
