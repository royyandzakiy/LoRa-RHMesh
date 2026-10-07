#pragma once

// Host simulator only. Boards compile this file empty.
#ifndef ARDUINO

#include "../Node.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "SimEther.h"

namespace rhmesh {

// One simulated board: a SimDriver, a Node and a thread that keeps polling,
// so the node forwards and ACKs like a powered-on device. Node isn't thread
// safe, so send() runs as a job on the node's own thread.
class SimNode {
 public:
  struct Received {
    uint8_t from;
    std::string text;
    uint8_t hops;
    int16_t rssi;
  };

  SimNode(SimEther& ether, uint8_t address);
  ~SimNode();

  SimNode(const SimNode&) = delete;
  SimNode& operator=(const SimNode&) = delete;

  Node::SendResult send(uint8_t dest, const std::string& text);
  Node::SendResult send(uint8_t dest, const uint8_t* data, uint8_t len);

  // Next hop towards dest, or -1 if the routing table has no valid route.
  int nextHopTo(uint8_t dest);

  // Runs fn on the node's thread and waits for it, for anything else on Node.
  void run(std::function<void(Node&)> fn);

  std::vector<Received> received();
  // Waits until a message from `from` has arrived, false on timeout.
  bool waitForMessageFrom(uint8_t from, unsigned timeoutMs);

  uint8_t address() const { return node_.address(); }
  Node& node() { return node_; }

  // Prints every received message to stdout, prefixed with this node's address.
  void setVerbose(bool verbose) { verbose_ = verbose; }

 private:
  static void onMessage(const Node::Message& msg, void* ctx);
  void loop();

  SimEther& ether_;
  SimDriver driver_;
  Node node_;
  std::thread thread_;
  std::atomic<bool> running_{true};
  std::atomic<bool> verbose_{false};

  std::mutex jobsMutex_;
  std::queue<std::function<void()>> jobs_;

  std::mutex rxMutex_;
  std::condition_variable rxCv_;
  std::vector<Received> received_;
};

}  // namespace rhmesh

#endif  // ARDUINO
