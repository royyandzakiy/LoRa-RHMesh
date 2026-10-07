#include <Arduino.h>
#include <LoRaRHMesh.h>
#include <RH_RF95.h>

RH_RF95 radio(RFM95_CS, RFM95_INT);
rhmesh::Node node(radio, 1);

void setup() {
  rhmesh::resetRadio();
  node.init();
}

void loop() {
  const uint8_t msg[] = "hi";
  node.send(254, msg, sizeof(msg));
  node.poll(1000);
}
