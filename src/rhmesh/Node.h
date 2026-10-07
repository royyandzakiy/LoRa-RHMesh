#pragma once

#include <RHMesh.h>

namespace rhmesh {

// Application layer on top of RHMesh: end-to-end ACKs and bounded send/receive.
// Depends only on RadioHead, so the same code runs on a board and in the simulator.
class Node {
 public:
  // Node 255 is RH_BROADCAST_ADDRESS: messages to it are never routed or ACKed.
  static constexpr uint8_t kMaxNodeAddress = 254;

  // Every message starts with a type byte and a sequence byte.
  static constexpr uint8_t kHeaderLen = 2;
  // Upper bound for buffers. The real limit depends on the radio, see maxPayload().
  static constexpr uint8_t kMaxPayload = RH_MESH_MAX_MESSAGE_LEN - kHeaderLen;

  enum class SendResult : uint8_t {
    Delivered,   // final node replied with an end-to-end ACK
    NoAck,       // first hop accepted it, but no end-to-end ACK arrived in time
    NoRoute,     // route discovery found no path to the destination
    HopFailed,   // next hop did not ACK (off the air or out of range)
    TooLong,     // payload larger than maxPayload()
    BadAddress,  // destination is the broadcast address or this node
  };

  struct Message {
    uint8_t from;
    const uint8_t* data;  // not null-terminated, use len
    uint8_t len;
    uint8_t hops;
    int16_t rssi;  // of the last hop, not the original sender
  };

  using MessageHandler = void (*)(const Message& msg, void* ctx);

  Node(RHGenericDriver& driver, uint8_t address);

  bool init();

  // Per-hop ACK wait and retry count used by RHReliableDatagram (defaults 200 ms, 3).
  void setHopTimeout(uint16_t ms) { mesh_.setTimeout(ms); }
  void setHopRetries(uint8_t retries) { mesh_.setRetries(retries); }

  // How long send() waits for the end-to-end ACK after the first hop succeeds.
  void setAckTimeout(uint16_t ms) { ackTimeoutMs_ = ms; }

  // How long send() keeps trying to find a route. RadioHead gives up on route discovery
  // after a fixed 4 s, too short for slow modem settings such as SF12. Above 0, send()
  // keeps listening for a late reply and retries discovery until this runs out.
  // 0 (default) means a single discovery attempt.
  void setRouteTimeout(uint32_t ms) { routeTimeoutMs_ = ms; }

  void onMessage(MessageHandler handler, void* ctx = nullptr);

  // Blocks until the end-to-end ACK arrives or the ACK timeout runs out.
  // Messages from other nodes that arrive meanwhile are still delivered and ACKed.
  SendResult send(uint8_t dest, const uint8_t* data, uint8_t len);

  // Receives and handles at most one message. Returns true if one arrived.
  // Also forwards messages for other nodes, so every node must keep calling it.
  bool poll(uint16_t timeoutMs);

  // Next hop towards dest from the routing table, false if no valid route is known.
  bool nextHopTo(uint8_t dest, uint8_t* nextHop);

  // Largest payload send() accepts. RH_RF95 carries 251 bytes, less than the
  // 255 RHMesh assumes, so this is smaller than kMaxPayload on real radios.
  uint8_t maxPayload();

  uint8_t address() const { return address_; }
  RHMesh& mesh() { return mesh_; }

  static const char* resultName(SendResult result);

 private:
  enum MsgType : uint8_t { kData = 1, kAck = 2 };

  // handleIncoming() results that are not an ACK sequence number.
  static constexpr int kNothing = -1;
  static constexpr int kOther = -2;

  // Returns the seq of an ACK from ackFrom, kOther for anything else received,
  // or kNothing if the timeout ran out.
  int handleIncoming(uint16_t timeoutMs, uint8_t ackFrom);

  // RHMesh::sendtoWait, retried while no route is found and routeTimeoutMs_ allows.
  uint8_t sendRouted(uint8_t dest, uint8_t len);

  RHGenericDriver& driver_;
  RHMesh mesh_;
  uint8_t address_;
  uint8_t nextSeq_ = 0;
  uint16_t ackTimeoutMs_ = 3000;
  uint32_t routeTimeoutMs_ = 0;
  MessageHandler handler_ = nullptr;
  void* handlerCtx_ = nullptr;
  uint8_t rxBuf_[RH_MESH_MAX_MESSAGE_LEN];
  uint8_t txBuf_[RH_MESH_MAX_MESSAGE_LEN];
};

}  // namespace rhmesh
