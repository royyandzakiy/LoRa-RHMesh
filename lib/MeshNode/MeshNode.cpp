#include "MeshNode.h"

#include <string.h>

MeshNode::MeshNode(RHGenericDriver& driver, uint8_t address)
    : driver_(driver), mesh_(driver, address), address_(address) {}

bool MeshNode::init() { return mesh_.init(); }

void MeshNode::onMessage(MessageHandler handler, void* ctx) {
  handler_ = handler;
  handlerCtx_ = ctx;
}

MeshNode::SendResult MeshNode::send(uint8_t dest, const uint8_t* data, uint8_t len) {
  if (dest == RH_BROADCAST_ADDRESS || dest == address_) return SendResult::BadAddress;
  if (len > kMaxPayload) return SendResult::TooLong;

  const uint8_t seq = nextSeq_++;
  txBuf_[0] = kData;
  txBuf_[1] = seq;
  memcpy(txBuf_ + kHeaderLen, data, len);

  switch (mesh_.sendtoWait(txBuf_, len + kHeaderLen, dest)) {
    case RH_ROUTER_ERROR_NONE:
      break;
    case RH_ROUTER_ERROR_NO_ROUTE:
      return SendResult::NoRoute;
    case RH_ROUTER_ERROR_INVALID_LENGTH:
      return SendResult::TooLong;
    default:
      return SendResult::HopFailed;
  }

  const unsigned long start = millis();
  unsigned long elapsed;
  while ((elapsed = millis() - start) < ackTimeoutMs_) {
    if (handleIncoming(ackTimeoutMs_ - elapsed, dest) == seq) return SendResult::Delivered;
  }
  return SendResult::NoAck;
}

bool MeshNode::poll(uint16_t timeoutMs) {
  return handleIncoming(timeoutMs, RH_BROADCAST_ADDRESS) != kNothing;
}

int MeshNode::handleIncoming(uint16_t timeoutMs, uint8_t ackFrom) {
  // recvfromAckTimeout uses len as both buffer size and received length.
  uint8_t len = sizeof(rxBuf_);
  uint8_t from, dest, hops;
  if (!mesh_.recvfromAckTimeout(rxBuf_, &len, timeoutMs, &from, &dest, nullptr, nullptr, &hops)) {
    return kNothing;
  }
  // Read before replying, sending the ACK receives a hop ACK and overwrites it.
  const int16_t rssi = driver_.lastRssi();

  if (len < kHeaderLen) return kOther;
  const uint8_t type = rxBuf_[0];
  const uint8_t seq = rxBuf_[1];

  if (type == kAck) return from == ackFrom ? seq : kOther;
  if (type != kData) return kOther;

  // ACK first, the sender's end-to-end timer is already running.
  if (dest == address_) {
    uint8_t ack[kHeaderLen] = {kAck, seq};
    mesh_.sendtoWait(ack, sizeof(ack), from);
  }

  if (handler_) {
    const Message msg{from, rxBuf_ + kHeaderLen, static_cast<uint8_t>(len - kHeaderLen), hops, rssi};
    handler_(msg, handlerCtx_);
  }
  return kOther;
}

bool MeshNode::nextHopTo(uint8_t dest, uint8_t* nextHop) {
  RHRouter::RoutingTableEntry* route = mesh_.getRouteTo(dest);
  if (!route || route->state != RHRouter::Valid) return false;
  *nextHop = route->next_hop;
  return true;
}

const char* MeshNode::resultName(SendResult result) {
  switch (result) {
    case SendResult::Delivered:
      return "delivered";
    case SendResult::NoAck:
      return "no end-to-end ACK";
    case SendResult::NoRoute:
      return "no route";
    case SendResult::HopFailed:
      return "next hop did not ACK";
    case SendResult::TooLong:
      return "payload too long";
    case SendResult::BadAddress:
      return "bad destination address";
  }
  return "unknown";
}
