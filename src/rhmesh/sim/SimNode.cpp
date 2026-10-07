// Host simulator only. Boards compile this file empty.
#ifndef ARDUINO

#include "SimNode.h"

#include <chrono>
#include <cstdio>

namespace rhmesh {

// Short poll so queued jobs start quickly.
static constexpr uint16_t kPollMs = 20;

SimNode::SimNode(SimEther& ether, uint8_t address) : ether_(ether), driver_(ether), node_(driver_, address) {
  node_.init();
  node_.onMessage(&SimNode::onMessage, this);
  thread_ = std::thread(&SimNode::loop, this);
}

SimNode::~SimNode() {
  running_ = false;
  thread_.join();
  ether_.detach(&driver_);
}

void SimNode::loop() {
  while (running_) {
    std::function<void()> job;
    {
      std::lock_guard<std::mutex> lock(jobsMutex_);
      if (!jobs_.empty()) {
        job = std::move(jobs_.front());
        jobs_.pop();
      }
    }
    if (job) {
      job();
    } else {
      node_.poll(kPollMs);
    }
  }
}

void SimNode::run(std::function<void(Node&)> fn) {
  std::promise<void> done;
  {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    jobs_.push([&] {
      fn(node_);
      done.set_value();
    });
  }
  done.get_future().wait();
}

Node::SendResult SimNode::send(uint8_t dest, const uint8_t* data, uint8_t len) {
  Node::SendResult result;
  run([&](Node& n) { result = n.send(dest, data, len); });
  return result;
}

Node::SendResult SimNode::send(uint8_t dest, const std::string& text) {
  return send(dest, reinterpret_cast<const uint8_t*>(text.data()), static_cast<uint8_t>(text.size()));
}

int SimNode::nextHopTo(uint8_t dest) {
  int hop = -1;
  run([&](Node& n) {
    uint8_t h;
    if (n.nextHopTo(dest, &h)) hop = h;
  });
  return hop;
}

void SimNode::onMessage(const Node::Message& msg, void* ctx) {
  SimNode* self = static_cast<SimNode*>(ctx);
  Received r{msg.from, std::string(reinterpret_cast<const char*>(msg.data), msg.len), msg.hops, msg.rssi};
  if (self->verbose_) {
    std::printf("[node %3d] from %d: \"%s\" rssi %d, %d hop(s)\n", self->address(), r.from, r.text.c_str(),
                r.rssi, r.hops);
  }
  {
    std::lock_guard<std::mutex> lock(self->rxMutex_);
    self->received_.push_back(std::move(r));
  }
  self->rxCv_.notify_all();
}

std::vector<SimNode::Received> SimNode::received() {
  std::lock_guard<std::mutex> lock(rxMutex_);
  return received_;
}

bool SimNode::waitForMessageFrom(uint8_t from, unsigned timeoutMs) {
  std::unique_lock<std::mutex> lock(rxMutex_);
  return rxCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
    for (const Received& r : received_) {
      if (r.from == from) return true;
    }
    return false;
  });
}

}  // namespace rhmesh

#endif  // ARDUINO
