# Bugs

Known issues and what happened to them. Boards use unpatched RadioHead, with workarounds
in `rhmesh::Node`. The simulator's RadioHead copy has two small patches, listed in
[`extras/sim-radiohead/PATCHES.md`](extras/sim-radiohead/PATCHES.md).

## Fixed

- **`sendtoWait` returns `RH_ROUTER_ERROR_NONE` with no other node running.**
  The end node used address 255, which is `RH_BROADCAST_ADDRESS`. RHMesh doesn't route
  messages to it, and RHReliableDatagram doesn't wait for an ACK, so the send always
  "succeeded". The end node is now 254, and `Node::send()` refuses 255.
  Covered by `test_broadcast_address_rejected` and `test_no_route_when_alone`.

- **Success reported for messages that never arrived.** `sendtoWait` only confirms the
  next hop. `rhmesh::Node` adds an end-to-end ACK. Covered by `test_no_ack_when_destination_dies`.

- **Received messages printed with `sprintf("%s")`** on a buffer that isn't
  null-terminated, and the receive length not reset between calls. Payloads are now
  handled with explicit lengths. Covered by `test_max_payload_round_trips`.

- **Full-size messages fail on the RFM95.** RHMesh allows 249 bytes, but the RFM95
  carries 251 per frame including 6 bytes of mesh headers. `Node::maxPayload()` uses
  the driver's limit (243). Covered by `test_max_payload_round_trips`.

- **Multi-hop route discovery fails at SF12.** RadioHead waits a fixed 4 s for a route
  (`RH_MESH_ARP_TIMEOUT`, not configurable upstream), less than two frames take on air at
  SF12. A reply that arrives later is still learned, so `Node::setRouteTimeout()` keeps
  listening and retries discovery. Covered by `test_route_discovery_on_slow_link`, not yet
  confirmed on hardware.

- **Nodes forward frames they only overheard** when the radio driver isn't a global
  (RadioHead leaves `_promiscuous` uninitialised). `Node::init()` sets it. Covered by
  `test_routes_over_line_topology`.

- **`#define RH_TEST_NETWORK` in a sketch does nothing.** It has to be a build flag,
  see the `testnet-*` envs.

## Open

- **`sendtoWait` sometimes stalls.** Seen on hardware during the thesis tests, root
  cause unknown. The watchdog (`WDT_TIMEOUT_S`) restarts the board when it happens. The
  simulator hasn't reproduced it, which points at the radio side (for example a missed
  DIO0 interrupt) rather than the routing code. If you hit it, please open an issue with
  the serial log.

- **Route discovery may truncate a reply** (upstream, not reproduced). `RHMesh::doArp()`
  doesn't reset its receive length between frames, so a short frame received first
  could make a longer route reply be cut off. Only the next hop is used from the
  reply, so the effect should be small.
