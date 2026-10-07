// Forced topology: pass -D RH_TEST_NETWORK=N as a build flag (see platformio.ini).
// A #define here has no effect, RHRouter.cpp is compiled separately.

#include <Arduino.h>
#include <MeshNode.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <esp_task_wdt.h>

#define RF95_FREQ 915.0
#define WDT_TIMEOUT_S 15

// Default pinout is the ESP32 DOIT devkit, platformio.ini overrides it per board.
#ifndef RFM95_CS
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2
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

const unsigned long sendIntervalMs_ = 3000;

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
  // Hardware reset so the modem starts clean after an ESP32-only reboot (e.g. watchdog).
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!meshNode_.init()) Serial.println("init failed");
  rfm95Modem_.setTxPower(23, false);
  rfm95Modem_.setFrequency(RF95_FREQ);
  rfm95Modem_.setCADTimeout(500);
}
