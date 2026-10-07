#pragma once

#include <RHGenericDriver.h>

#include <deque>
#include <mutex>
#include <random>
#include <vector>

class SimDriver;

// Shared radio channel for simulated nodes. A frame sent by one node reaches every
// node it has a link to. No collisions or half-duplex, every frame is heard cleanly.
class SimEther {
 public:
  struct Frame {
    uint8_t to, from, id, flags;
    std::vector<uint8_t> data;
    int16_t rssi;
  };

  // Frames are delayed this long in send(), like LoRa time on air.
  explicit SimEther(unsigned airtimeMs = 0) : airtimeMs_(airtimeMs) {}

  void attach(SimDriver* driver);
  void detach(SimDriver* driver);

  // Links are symmetric. With no links set, every node hears every other node.
  void link(uint8_t a, uint8_t b, int16_t rssi = -60);
  void unlink(uint8_t a, uint8_t b);
  // Probability (0..1) that a frame on this link is lost.
  void setDropRate(uint8_t a, uint8_t b, double rate);
  // A powered-off node neither sends nor receives.
  void setPowered(uint8_t address, bool on);

  void transmit(uint8_t fromAddress, Frame frame);
  bool pop(uint8_t address, bool promiscuous, Frame* out);

  // Prints every frame on the air: sender, header fields, length and who heard it.
  void setTrace(bool on) { trace_ = on; }

  unsigned airtimeMs() const { return airtimeMs_; }

 private:
  struct Link {
    uint8_t a, b;
    int16_t rssi;
    double drop;
  };

  Link* findLink(uint8_t a, uint8_t b);
  SimDriver* findDriver(uint8_t address);

  std::mutex mutex_;
  std::vector<SimDriver*> drivers_;
  std::vector<Link> links_;
  bool fullMesh_ = true;
  bool trace_ = false;
  unsigned airtimeMs_;
  std::mt19937 rng_{1234};
};

// RadioHead driver that sends through a SimEther instead of a radio.
class SimDriver : public RHGenericDriver {
 public:
  // Same usable payload as RH_RF95, so length limits match the hardware.
  static constexpr uint8_t kMaxMessageLen = 251;

  explicit SimDriver(SimEther& ether) : ether_(ether) {}

  bool init() override;
  bool available() override;
  bool recv(uint8_t* buf, uint8_t* len) override;
  bool send(const uint8_t* data, uint8_t len) override;
  uint8_t maxMessageLength() override { return kMaxMessageLen; }

  uint8_t thisAddress() const { return _thisAddress; }
  bool powered() const { return powered_; }

 private:
  friend class SimEther;

  SimEther& ether_;
  std::deque<SimEther::Frame> rxQueue_;  // guarded by the ether's mutex
  SimEther::Frame current_;
  bool hasCurrent_ = false;
  bool powered_ = true;
};
