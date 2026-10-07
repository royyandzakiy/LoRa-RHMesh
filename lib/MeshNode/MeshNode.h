#pragma once

#include <RHMesh.h>

// Application layer on top of RHMesh: end-to-end ACKs and bounded send/receive.
// Depends only on RadioHead, so the same code runs on ESP32 and in a host simulator.
class MeshNode {
 public:
  // Node 255 is RH_BROADCAST_ADDRESS: messages to it are never routed or ACKed.
  static constexpr uint8_t kMaxNodeAddress = 254;

  // Every message starts with a type byte and a sequence byte.
  static constexpr uint8_t kHeaderLen = 2;
  static constexpr uint8_t kMaxPayload = RH_MESH_MAX_MESSAGE_LEN - kHeaderLen;

  enum class SendResult : uint8_t {
    Delivered,   // final node replied with an end-to-end ACK
    NoAck,       // first hop accepted it, but no end-to-end ACK arrived in time
    NoRoute,     // route discovery found no path to the destination
    HopFailed,   // next hop did not ACK (off the air or out of range)
    TooLong,     // payload larger than kMaxPayload
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

  MeshNode(RHGenericDriver& driver, uint8_t address);

  bool init();

  // Per-hop ACK wait and retry count used by RHReliableDatagram (defaults 200 ms, 3).
  void setHopTimeout(uint16_t ms) { mesh_.setTimeout(ms); }
  void setHopRetries(uint8_t retries) { mesh_.setRetries(retries); }

  // How long send() waits for the end-to-end ACK after the first hop succeeds.
  void setAckTimeout(uint16_t ms) { ackTimeoutMs_ = ms; }

  void onMessage(MessageHandler handler, void* ctx = nullptr);

  // Blocks until the end-to-end ACK arrives or the ACK timeout runs out.
  // Messages from other nodes that arrive meanwhile are still delivered and ACKed.
  SendResult send(uint8_t dest, const uint8_t* data, uint8_t len);

  // Receives and handles at most one message. Returns true if one arrived.
  bool poll(uint16_t timeoutMs);

  // Next hop towards dest from the routing table, false if no valid route is known.
  bool nextHopTo(uint8_t dest, uint8_t* nextHop);

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

  RHGenericDriver& driver_;
  RHMesh mesh_;
  uint8_t address_;
  uint8_t nextSeq_ = 0;
  uint16_t ackTimeoutMs_ = 3000;
  MessageHandler handler_ = nullptr;
  void* handlerCtx_ = nullptr;
  uint8_t rxBuf_[RH_MESH_MAX_MESSAGE_LEN];
  uint8_t txBuf_[RH_MESH_MAX_MESSAGE_LEN];
};
