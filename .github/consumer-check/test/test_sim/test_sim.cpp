#include <LoRaRHMesh.h>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_two_nodes() {
  rhmesh::SimEther ether;
  rhmesh::SimNode a(ether, 1), b(ether, 2);
  TEST_ASSERT_EQUAL_STRING("delivered", rhmesh::Node::resultName(a.send(2, "hi")));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_two_nodes);
  return UNITY_END();
}
