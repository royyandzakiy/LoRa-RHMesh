// Mesh behaviour on simulated radios. Each test builds its own ether and nodes.
#include <SimNode.h>
#include <unity.h>

#include <memory>
#include <string>

using Result = MeshNode::SendResult;

static void assertResult(Result expected, Result actual) {
  TEST_ASSERT_EQUAL_STRING(MeshNode::resultName(expected), MeshNode::resultName(actual));
}

// 1 - 2 - 3 - 4, every node only hears its neighbours.
static void linkLine(SimEther& ether) {
  ether.link(1, 2);
  ether.link(2, 3);
  ether.link(3, 4);
}

void setUp() {}
void tearDown() {}

void test_direct_delivery() {
  SimEther ether;
  SimNode a(ether, 1), b(ether, 254);

  assertResult(Result::Delivered, a.send(254, "hello"));
  TEST_ASSERT_TRUE(b.waitForMessageFrom(1, 1000));
  TEST_ASSERT_EQUAL_STRING("hello", b.received()[0].text.c_str());
  TEST_ASSERT_EQUAL(254, a.nextHopTo(254));
}

void test_routes_over_line_topology() {
  SimEther ether;
  linkLine(ether);
  SimNode n1(ether, 1), n2(ether, 2), n3(ether, 3), n4(ether, 4);

  assertResult(Result::Delivered, n1.send(4, "over three hops"));
  TEST_ASSERT_TRUE(n4.waitForMessageFrom(1, 1000));
  TEST_ASSERT_EQUAL(2, n4.received()[0].hops);  // forwarded by 2 and 3

  // Each node only knows the next hop, not the full path.
  TEST_ASSERT_EQUAL(2, n1.nextHopTo(4));
  TEST_ASSERT_EQUAL(3, n2.nextHopTo(4));
  // Route discovery also taught node 4 the way back.
  TEST_ASSERT_EQUAL(3, n4.nextHopTo(1));
}

void test_reroutes_around_dead_node() {
  // Diamond: 1 reaches 4 through either 2 or 3.
  SimEther ether;
  ether.link(1, 2);
  ether.link(1, 3);
  ether.link(2, 4);
  ether.link(3, 4);
  SimNode n1(ether, 1), n2(ether, 2), n3(ether, 3), n4(ether, 4);

  assertResult(Result::Delivered, n1.send(4, "first"));
  const int firstHop = n1.nextHopTo(4);
  TEST_ASSERT_TRUE(firstHop == 2 || firstHop == 3);

  ether.setPowered(firstHop, false);

  // The first send may fail on the stale route, RHMesh then drops it and rediscovers.
  Result result = Result::NoAck;
  for (int attempt = 0; attempt < 3 && result != Result::Delivered; attempt++) {
    result = n1.send(4, "second");
  }
  assertResult(Result::Delivered, result);
  TEST_ASSERT_EQUAL(firstHop == 2 ? 3 : 2, n1.nextHopTo(4));
}

void test_no_route_when_alone() {
  SimEther ether;
  SimNode lonely(ether, 1);

  assertResult(Result::NoRoute, lonely.send(254, "anyone?"));
}

void test_broadcast_address_rejected() {
  // The old end node address. RHMesh would broadcast it one hop with no ACK and report success.
  SimEther ether;
  SimNode a(ether, 1), b(ether, 2);

  assertResult(Result::BadAddress, a.send(255, "to the end node"));
  assertResult(Result::BadAddress, a.send(1, "to myself"));
}

void test_no_ack_when_destination_dies() {
  SimEther ether;
  linkLine(ether);
  SimNode n1(ether, 1), n2(ether, 2), n3(ether, 3), n4(ether, 4);
  assertResult(Result::Delivered, n1.send(4, "route warm-up"));

  ether.setPowered(4, false);
  n1.run([](MeshNode& n) { n.setAckTimeout(1000); });

  // Node 2 still ACKs the first hop, only the end-to-end ACK shows the message was lost.
  assertResult(Result::NoAck, n1.send(4, "lost"));
}

void test_max_payload_round_trips() {
  SimEther ether;
  SimNode a(ether, 1), b(ether, 2);

  // 251 radio bytes - 5 router header - 1 mesh header - 2 MeshNode header.
  TEST_ASSERT_EQUAL(243, a.node().maxPayload());

  // Binary payload with zero bytes and no terminator must arrive intact.
  std::string payload(243, '\0');
  for (size_t i = 0; i < payload.size(); i++) payload[i] = static_cast<char>(i);

  assertResult(Result::Delivered, a.send(2, payload));
  TEST_ASSERT_TRUE(b.waitForMessageFrom(1, 1000));
  TEST_ASSERT_TRUE(b.received()[0].text == payload);

  assertResult(Result::TooLong, a.send(2, std::string(244, 'x')));
}

void test_lossy_link_still_delivers() {
  // Hop ACKs and retries hide a 30% loss rate on every link.
  SimEther ether;
  linkLine(ether);
  ether.setDropRate(1, 2, 0.3);
  ether.setDropRate(2, 3, 0.3);
  ether.setDropRate(3, 4, 0.3);
  SimNode n1(ether, 1), n2(ether, 2), n3(ether, 3), n4(ether, 4);
  n1.run([](MeshNode& n) { n.setHopRetries(6); });
  n2.run([](MeshNode& n) { n.setHopRetries(6); });
  n3.run([](MeshNode& n) { n.setHopRetries(6); });
  n4.run([](MeshNode& n) { n.setHopRetries(6); });

  int delivered = 0;
  for (int i = 0; i < 5; i++) {
    if (n1.send(4, "msg " + std::to_string(i)) == Result::Delivered) delivered++;
  }
  TEST_ASSERT_GREATER_OR_EQUAL(3, delivered);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_direct_delivery);
  RUN_TEST(test_routes_over_line_topology);
  RUN_TEST(test_reroutes_around_dead_node);
  RUN_TEST(test_no_route_when_alone);
  RUN_TEST(test_broadcast_address_rejected);
  RUN_TEST(test_no_ack_when_destination_dies);
  RUN_TEST(test_max_payload_round_trips);
  RUN_TEST(test_lossy_link_still_delivers);
  return UNITY_END();
}
