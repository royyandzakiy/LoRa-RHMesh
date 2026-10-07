// Runs the README topology on simulated radios: nodes 1-3 send to the end node 254,
// and only node 1 is in range of it. Usage: pio run -e sim-demo -t exec
// Pass --trace (pio run -e sim-demo -t exec -a --trace) to print every frame on the air.
#include <SimNode.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

static constexpr uint8_t kEndNode = 254;
static constexpr int kRounds = 4;
// Rough SF7/125 kHz time on air for a short message.
static constexpr unsigned kAirtimeMs = 30;

int main(int argc, char** argv) {
  const bool trace = argc > 1 && std::strcmp(argv[1], "--trace") == 0;

  // 3 - 2 - 1 - 254
  SimEther ether(kAirtimeMs);
  ether.link(3, 2, -95);
  ether.link(2, 1, -80);
  ether.link(1, kEndNode, -70);
  ether.setTrace(trace);

  SimNode end(ether, kEndNode);
  end.setVerbose(true);
  std::vector<std::unique_ptr<SimNode>> senders;
  for (uint8_t addr : {1, 2, 3}) senders.push_back(std::make_unique<SimNode>(ether, addr));

  std::printf("Topology: 3 - 2 - 1 - %d, %d rounds\n\n", kEndNode, kRounds);
  for (int round = 0; round < kRounds; round++) {
    for (auto& node : senders) {
      const std::string msg = "Hello from node " + std::to_string(node->address()) + " #" + std::to_string(round);
      std::printf("[node %3d] sending \"%s\" to %d...\n", node->address(), msg.c_str(), kEndNode);
      const MeshNode::SendResult result = node->send(kEndNode, msg);
      std::printf("[node %3d] %s (next hop %d)\n", node->address(), MeshNode::resultName(result),
                  node->nextHopTo(kEndNode));
    }
    std::printf("\n");
  }

  // Node 1 goes off the air. Node 2 sees its hop fail, but node 3's first hop still
  // works, so only the missing end-to-end ACK shows its message was lost.
  std::printf("Powering off node 1\n");
  ether.setPowered(1, false);
  for (auto& node : senders) {
    if (node->address() == 1) continue;
    node->run([](MeshNode& n) { n.setAckTimeout(1500); });
    const MeshNode::SendResult result = node->send(kEndNode, "anyone there?");
    std::printf("[node %3d] %s\n", node->address(), MeshNode::resultName(result));
  }
  return 0;
}
