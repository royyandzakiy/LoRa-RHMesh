// A mesh node: sends "Hello" to the end node every few seconds and logs what it receives.
// Flash it to two or more boards with a different SELF_ADDRESS each. The end node only
// listens and replies with an end-to-end ACK.
//
// Every setting below can also come from a build flag (see platformio.ini in the repo).
// Forced topology for desk tests needs RH_TEST_NETWORK as a build flag, a #define here
// has no effect because RadioHead's RHRouter.cpp is compiled separately.

// ---- settings, change SELF_ADDRESS for every board --------------------------------
#ifndef SELF_ADDRESS
#define SELF_ADDRESS 1
#endif
// Where this node sends to. 0-254, 255 is RadioHead's broadcast address.
#ifndef TARGET_ADDRESS
#define TARGET_ADDRESS 254
#endif
// The node that only listens and ACKs.
#ifndef ENDNODE_ADDRESS
#define ENDNODE_ADDRESS 254
#endif

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
// Total time to keep looking for a route. 0 is one discovery attempt (RadioHead's fixed 4 s).
#ifndef ROUTE_TIMEOUT_MS
#define ROUTE_TIMEOUT_MS 0
#endif
// Longer than the slowest send (route plus ACK timeout), or the watchdog resets mid-send.
#ifndef WDT_TIMEOUT_S
#define WDT_TIMEOUT_S 15
#endif
// MODEM_CONFIG: optional, one of RH_RF95::ModemConfigChoice, e.g. Bw125Cr48Sf4096.
// Pins (RFM95_CS/RST/INT) and RF95_FREQ: see rhmesh/Board.h.
// -----------------------------------------------------------------------------------

#include <LoRaRHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>

#ifdef ESP32
#include <esp_task_wdt.h>
#endif

const uint8_t selfAddress_ = SELF_ADDRESS;
const uint8_t targetAddress_ = TARGET_ADDRESS;

static_assert(selfAddress_ <= rhmesh::Node::kMaxNodeAddress, "255 is the broadcast address");
static_assert(targetAddress_ <= rhmesh::Node::kMaxNodeAddress, "255 is the broadcast address");

RH_RF95 rfm95Modem_(RFM95_CS, RFM95_INT);
rhmesh::Node meshNode_(rfm95Modem_, selfAddress_);

void watchdogStart();
void watchdogFeed();
void rhSetup();
void onMessage(const rhmesh::Node::Message& msg, void*);
void sendHello();

void setup() {
  Serial.begin(115200);
  watchdogStart();

  rhSetup();
  meshNode_.onMessage(onMessage);
  Serial.printf(" ---------------- LORA NODE %d INIT ---------------- \n", selfAddress_);
}

void loop() {
  static unsigned long lastSend = 0;

  if (selfAddress_ != ENDNODE_ADDRESS && millis() - lastSend > SEND_INTERVAL_MS) {
    sendHello();
    lastSend = millis();
  }

  meshNode_.poll(500);
  watchdogFeed();
}

void sendHello() {
  static uint32_t counter = 0;
  char msg[rhmesh::Node::kMaxPayload];
  const int len = snprintf(msg, sizeof(msg), "Hello from node %d #%lu", selfAddress_,
                           static_cast<unsigned long>(counter++));

  Serial.printf("Sending \"%s\" to %d... ", msg, targetAddress_);
  const rhmesh::Node::SendResult result =
      meshNode_.send(targetAddress_, reinterpret_cast<const uint8_t*>(msg), len);
  Serial.printf("%s", rhmesh::Node::resultName(result));

  uint8_t nextHop;
  if (meshNode_.nextHopTo(targetAddress_, &nextHop)) {
    Serial.printf(" (next hop %d)", nextHop);
  }
  Serial.println();
  watchdogFeed();
}

void onMessage(const rhmesh::Node::Message& msg, void*) {
  // Payload is not null-terminated, print it with an explicit length.
  Serial.printf("[%d] \"%.*s\" rssi %d, %d hop(s)\n", msg.from, msg.len,
                reinterpret_cast<const char*>(msg.data), msg.rssi, msg.hops);
}

void rhSetup() {
  rhmesh::resetRadio();

  if (!meshNode_.init()) Serial.println("init failed, check the wiring");
  rfm95Modem_.setTxPower(23, false);
  rfm95Modem_.setFrequency(RF95_FREQ);
  rfm95Modem_.setCADTimeout(500);
#ifdef MODEM_CONFIG
  rfm95Modem_.setModemConfig(RH_RF95::MODEM_CONFIG);
#endif

  meshNode_.setHopTimeout(HOP_TIMEOUT_MS);
  meshNode_.setAckTimeout(ACK_TIMEOUT_MS);
  meshNode_.setRouteTimeout(ROUTE_TIMEOUT_MS);
}

// Restarts the board if a send ever hangs (see BUGS.md).
void watchdogStart() {
#if defined(ESP32) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // Core 3 starts the task watchdog itself, so only its timeout changes.
  esp_task_wdt_config_t config = {};
  config.timeout_ms = WDT_TIMEOUT_S * 1000;
  config.trigger_panic = true;
  esp_task_wdt_reconfigure(&config);
  esp_task_wdt_add(NULL);
#elif defined(ESP32)
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
#endif
}

void watchdogFeed() {
#ifdef ESP32
  esp_task_wdt_reset();
#endif
}
