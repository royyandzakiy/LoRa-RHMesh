// Point-to-point range test with the bare RH_RF95 driver: no addressing, no ACKs, no mesh.
// The sender transmits numbered packets, the receiver logs RSSI, SNR and packet loss.
// Walk the receiver away from the sender to find where a modem setting stops working.
//
// Build flags (see the range-test envs in platformio.ini):
//   RANGE_TEST_SENDER  build the sender, otherwise the receiver
//   MODEM_CONFIG       one of RH_RF95::ModemConfigChoice, both sides must match

#include <BoardConfig.h>
#include <RH_RF95.h>
#include <SPI.h>

#ifndef MODEM_CONFIG
#define MODEM_CONFIG Bw125Cr45Sf128
#endif

#define STR_(x) #x
#define STR(x) STR_(x)

const uint32_t packetCount_ = 200;
const unsigned long sendIntervalMs_ = 500;

RH_RF95 rfm95Modem_(RFM95_CS, RFM95_INT);

void setup() {
  Serial.begin(115200);
  resetRadio();

  if (!rfm95Modem_.init()) {
    Serial.println("LoRa radio init failed, check the wiring");
    while (true) delay(1000);
  }
  rfm95Modem_.setFrequency(RF95_FREQ);
  rfm95Modem_.setTxPower(23, false);
  rfm95Modem_.setModemConfig(RH_RF95::MODEM_CONFIG);

#ifdef RANGE_TEST_SENDER
  Serial.printf("Range test SENDER, %s, %lu packets\n", STR(MODEM_CONFIG), static_cast<unsigned long>(packetCount_));
#else
  Serial.printf("Range test RECEIVER, %s\n", STR(MODEM_CONFIG));
#endif
}

#ifdef RANGE_TEST_SENDER

void loop() {
  static uint32_t seq = 0;
  static unsigned long totalAirtimeMs = 0;

  if (seq >= packetCount_) {
    Serial.printf("Done, %lu packets, average %lu ms on air\n", static_cast<unsigned long>(seq),
                  totalAirtimeMs / seq);
    while (true) delay(1000);
  }

  // Sequence number for counting gaps, padded to 20 bytes so airtime matches a short real message.
  uint8_t packet[20] = {0};
  memcpy(packet, &seq, sizeof(seq));

  const unsigned long start = millis();
  rfm95Modem_.send(packet, sizeof(packet));
  rfm95Modem_.waitPacketSent();
  const unsigned long airtimeMs = millis() - start;
  totalAirtimeMs += airtimeMs;

  Serial.printf("sent #%lu, %lu ms on air\n", static_cast<unsigned long>(seq), airtimeMs);
  seq++;
  delay(sendIntervalMs_);
}

#else

void loop() {
  static uint32_t received = 0;
  static uint32_t lost = 0;
  static int64_t lastSeq = -1;

  if (!rfm95Modem_.available()) return;

  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (!rfm95Modem_.recv(buf, &len) || len < sizeof(uint32_t)) {
    Serial.println("bad packet");
    return;
  }

  uint32_t seq;
  memcpy(&seq, buf, sizeof(seq));
  // A smaller seq means the sender restarted, start counting again.
  if (lastSeq < 0 || seq <= lastSeq) {
    received = 0;
    lost = 0;
  } else {
    lost += seq - lastSeq - 1;
  }
  lastSeq = seq;
  received++;

  const float lossPct = 100.0f * lost / (received + lost);
  Serial.printf("#%lu rssi %d dBm, snr %d dB, lost %lu (%.1f%%)\n", static_cast<unsigned long>(seq),
                rfm95Modem_.lastRssi(), rfm95Modem_.lastSNR(), static_cast<unsigned long>(lost), lossPct);
}

#endif
