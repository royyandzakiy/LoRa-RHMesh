// Host simulator only. Boards compile this file empty.
#ifndef ARDUINO

#include "SimEther.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace rhmesh {

// A real radio holds one frame, the queue only absorbs thread scheduling jitter.
static constexpr size_t kRxQueueLen = 8;

void SimEther::attach(SimDriver* driver) {
  std::lock_guard<std::mutex> lock(mutex_);
  drivers_.push_back(driver);
}

void SimEther::detach(SimDriver* driver) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = drivers_.begin(); it != drivers_.end(); ++it) {
    if (*it == driver) {
      drivers_.erase(it);
      return;
    }
  }
}

void SimEther::link(uint8_t a, uint8_t b, int16_t rssi) {
  std::lock_guard<std::mutex> lock(mutex_);
  fullMesh_ = false;
  if (Link* l = findLink(a, b)) {
    l->rssi = rssi;
  } else {
    links_.push_back({a, b, rssi, 0.0});
  }
}

void SimEther::unlink(uint8_t a, uint8_t b) {
  std::lock_guard<std::mutex> lock(mutex_);
  fullMesh_ = false;
  for (auto it = links_.begin(); it != links_.end(); ++it) {
    if ((it->a == a && it->b == b) || (it->a == b && it->b == a)) {
      links_.erase(it);
      return;
    }
  }
}

void SimEther::setDropRate(uint8_t a, uint8_t b, double rate) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (Link* l = findLink(a, b)) l->drop = rate;
}

void SimEther::setPowered(uint8_t address, bool on) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (SimDriver* d = findDriver(address)) {
    d->powered_ = on;
    if (!on) d->rxQueue_.clear();
  }
}

void SimEther::transmit(uint8_t fromAddress, Frame frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  SimDriver* sender = findDriver(fromAddress);
  if (!sender || !sender->powered_) return;
  std::string heardBy;

  for (SimDriver* d : drivers_) {
    if (d == sender || !d->powered_) continue;
    const uint8_t to = d->_thisAddress;

    Frame copy = frame;
    copy.rssi = -60;
    if (!fullMesh_) {
      Link* l = findLink(fromAddress, to);
      if (!l) continue;
      if (l->drop > 0 && std::uniform_real_distribution<double>(0, 1)(rng_) < l->drop) continue;
      copy.rssi = l->rssi;
    }
    if (d->rxQueue_.size() < kRxQueueLen) {
      d->rxQueue_.push_back(std::move(copy));
      heardBy += " " + std::to_string(to);
    }
  }
  if (trace_) {
    std::printf("%8lu  %3d -> %3d  id %3d  flags 0x%02x  len %3u  heard by:%s\n", millis(), fromAddress, frame.to,
                frame.id, frame.flags, static_cast<unsigned>(frame.data.size()), heardBy.c_str());
  }
}

bool SimEther::pop(uint8_t address, bool promiscuous, Frame* out) {
  std::lock_guard<std::mutex> lock(mutex_);
  SimDriver* d = findDriver(address);
  if (!d) return false;
  // Drop frames addressed to other nodes, as the radio's header filter would.
  while (!d->rxQueue_.empty()) {
    Frame& f = d->rxQueue_.front();
    if (promiscuous || f.to == address || f.to == RH_BROADCAST_ADDRESS) {
      *out = std::move(f);
      d->rxQueue_.pop_front();
      return true;
    }
    d->rxQueue_.pop_front();
  }
  return false;
}

SimEther::Link* SimEther::findLink(uint8_t a, uint8_t b) {
  for (Link& l : links_) {
    if ((l.a == a && l.b == b) || (l.a == b && l.b == a)) return &l;
  }
  return nullptr;
}

SimDriver* SimEther::findDriver(uint8_t address) {
  for (SimDriver* d : drivers_) {
    if (d->_thisAddress == address) return d;
  }
  return nullptr;
}

bool SimDriver::init() {
  ether_.attach(this);
  return RHGenericDriver::init();
}

bool SimDriver::available() {
  if (hasCurrent_) return true;
  if (ether_.pop(_thisAddress, _promiscuous, &current_)) {
    hasCurrent_ = true;
    _rxHeaderTo = current_.to;
    _rxHeaderFrom = current_.from;
    _rxHeaderId = current_.id;
    _rxHeaderFlags = current_.flags;
    _lastRssi = current_.rssi;
    _rxGood++;
    return true;
  }
  // RadioHead's wait loops spin on available(), this keeps idle nodes off the CPU.
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  return false;
}

bool SimDriver::recv(uint8_t* buf, uint8_t* len) {
  if (!available()) return false;
  if (buf && len) {
    if (*len > current_.data.size()) *len = current_.data.size();
    memcpy(buf, current_.data.data(), *len);
  }
  hasCurrent_ = false;
  return true;
}

bool SimDriver::send(const uint8_t* data, uint8_t len) {
  if (len > kMaxMessageLen) return false;
  SimEther::Frame frame{_txHeaderTo, _txHeaderFrom, _txHeaderId, _txHeaderFlags,
                        std::vector<uint8_t>(data, data + len), 0};
  if (ether_.airtimeMs()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ether_.airtimeMs()));
  }
  ether_.transmit(_thisAddress, std::move(frame));
  _txGood++;
  return true;
}

}  // namespace rhmesh

#endif  // ARDUINO
