# LoRa RHMesh

[![CI](https://github.com/royyandzakiy/LoRa-RHMesh/actions/workflows/ci.yml/badge.svg)](https://github.com/royyandzakiy/LoRa-RHMesh/actions/workflows/ci.yml)

A LoRa mesh library for Arduino and PlatformIO, built on [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/)'s `RHMesh`. Nodes find routes to each other on their own and forward messages over several hops. On top of RHMesh it adds:

- **End-to-end delivery confirmation.** `send()` tells you whether the destination got the message, not just the next hop.
- **Route discovery that works at SF12.** RadioHead gives up after a fixed 4 s, which is shorter than two frames take on air at slow modem settings.
- **Fixes for RadioHead quirks**: address 255, payload limits that match the RFM95, an uninitialised driver flag.
- **A simulator**, so you can run and unit-test your mesh code on a PC with no radios.

Examples target ESP32 with an RFM95 (SX1276) module, including the TTGO T-Beam.

> This started as the code for my thesis. The original sketches are kept in the [`v0-thesis`](https://github.com/royyandzakiy/LoRa-RHMesh/tree/v0-thesis) tag.

- [Install](#install)
- [Quick start](#quick-start)
- [Test your mesh code in the simulator](#test-your-mesh-code-in-the-simulator)
- [How RHMesh routes](#how-rhmesh-routes)
- [Examples](#examples)
- [Settings](#settings)
- [Hardware](#hardware)
- [Developing this library](#developing-this-library)

## Install

**PlatformIO**, in `platformio.ini`:
```ini
lib_deps = https://github.com/royyandzakiy/LoRa-RHMesh.git
```
This also installs RadioHead 1.143 for Arduino builds.

**Arduino IDE:** install **RadioHead** (1.143.1 or newer) from the Library Manager, then this library: download the repository as a ZIP and add it with *Sketch → Include Library → Add .ZIP Library*.

## Quick start

```cpp
#include <LoRaRHMesh.h>
#include <RH_RF95.h>

RH_RF95 radio(RFM95_CS, RFM95_INT);  // pins from rhmesh/Board.h, or your own
rhmesh::Node node(radio, 1);         // this node's address, 0-254

void onMessage(const rhmesh::Node::Message& msg, void*) {
  Serial.printf("from %d: %.*s\n", msg.from, msg.len, reinterpret_cast<const char*>(msg.data));
}

void setup() {
  Serial.begin(115200);
  rhmesh::resetRadio();
  node.init();
  radio.setFrequency(915.0);
  node.onMessage(onMessage);
}

void loop() {
  const char text[] = "hello";
  auto result = node.send(254, reinterpret_cast<const uint8_t*>(text), sizeof(text) - 1);
  Serial.println(rhmesh::Node::resultName(result));  // "delivered", "no route", ...
  node.poll(3000);  // receive and forward for others, every node must keep calling this
}
```

`send()` blocks until the destination's end-to-end ACK arrives or the ACK timeout runs out. Messages that arrive in the meantime are still handled. The full sketch is [`examples/MeshNode`](examples/MeshNode/MeshNode.ino).

## Test your mesh code in the simulator

The simulator runs RHMesh nodes as threads on your PC, on a simulated radio channel where you decide who hears whom. It needs PlatformIO and a host C++ compiler: g++ on Linux or WSL, Xcode tools on macOS, or [MSYS2](https://www.msys2.org/) g++ on Windows.

Add a native env next to your board env:
```ini
[env:native]
platform = native
lib_deps = https://github.com/royyandzakiy/LoRa-RHMesh.git
build_flags = -D UNITY_EXCLUDE_SETJMP_H
```
`UNITY_EXCLUDE_SETJMP_H` lets a failed assert end the test normally, so the node threads are stopped.

Then write a test in `test/test_mesh/test_mesh.cpp` and run `pio test -e native`:
```cpp
#include <LoRaRHMesh.h>
#include <unity.h>
using namespace rhmesh;

void setUp() {}
void tearDown() {}

void test_routes_around_the_corner() {
  SimEther ether;          // optional: SimEther ether(1500) for 1.5 s per frame
  ether.link(1, 2);        // 1 - 2 - 3, node 1 can't hear node 3
  ether.link(2, 3);
  SimNode n1(ether, 1), n2(ether, 2), n3(ether, 3);

  TEST_ASSERT_EQUAL_STRING("delivered", Node::resultName(n1.send(3, "hi")));
  TEST_ASSERT_EQUAL(2, n1.nextHopTo(3));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_routes_around_the_corner);
  return UNITY_END();
}
```

`SimEther` is the radio channel: `link(a, b, rssi)`, `setDropRate(a, b, 0.3)` for a lossy link, `setPowered(addr, false)` to switch a node off, and `setTrace(true)` to print every frame. A `SimNode` is one board with its own thread. `SimNode::run()` runs code on that thread, for example `node.run([](rhmesh::Node& n) { n.setRouteTimeout(30000); })`.

The simulator models routing, not radio physics: there are no collisions, and every frame on a link arrives cleanly unless you add loss. It uses a copy of RadioHead's mesh code with two small fixes so several nodes can share one process, see [`extras/sim-radiohead/PATCHES.md`](extras/sim-radiohead/PATCHES.md). Boards use normal, unpatched RadioHead.

## How RHMesh routes

<img width="700" src="docs/topology-full.png">

In this 4-node example, the final node (address 254 in the code) is the end node, for example a gateway that later forwards everything to the cloud. Nodes 1-3 send to it, and can also forward for each other.

> Don't use address 255 for a node. It's RadioHead's broadcast address: messages to it are sent one hop with no route and no ACK, and RadioHead still reports success. `Node::send()` refuses it.

<img width="700" src="docs/topology-route.png">

When a node sends to an address it has no route for, RHMesh broadcasts a route request. Every node forwards it once and adds itself to the path, and the destination replies along that path. Each node on the way **only saves the next hop**, not the full path. In the picture, node 2 knows "to reach the final node, send to node 1". It doesn't know whether node 1 is the last hop or there are more.

### Two kinds of ACK
- **Hop ACK** (RadioHead): `sendtoWait` returns success as soon as the *next hop* acknowledges. That doesn't mean the destination got it.
- **End-to-end ACK** (this library): the destination sends a small ACK back to the original sender, and `Node::send()` waits for it:

| Result | Meaning |
|---|---|
| `delivered` | the destination confirmed it |
| `no end-to-end ACK` | the first hop took it, but it got lost further on, or the destination is down |
| `next hop did not ACK` | the next hop is off the air or out of range |
| `no route` | route discovery found no path within the route timeout |
| `payload too long` | more than `maxPayload()` bytes (243 on the RFM95) |
| `bad destination address` | 255 or this node's own address |

## Examples

| Sketch | What it shows |
|---|---|
| [`MeshNode`](examples/MeshNode/MeshNode.ino) | A mesh node: route discovery, multi-hop delivery, end-to-end ACK |
| [`RangeTest`](examples/RangeTest/RangeTest.ino) | The radio alone, no mesh: RSSI, SNR, packet loss and time on air for a modem setting |
| [`StaticRouting`](examples/StaticRouting/StaticRouting.ino) | `RHRouter` with routes written by hand, to compare with RHMesh finding them |
| [`extras/sim-demo`](extras/sim-demo/demo.cpp) | Four nodes on simulated radios, run with `pio run -e sim-demo -t exec` in this repo |

In the Arduino IDE, open them from *File → Examples → LoRa-RHMesh* and change the `#define`s at the top, for example `SELF_ADDRESS` for each board. In this repo each one also has PlatformIO envs, see [below](#developing-this-library).

## Settings

On `rhmesh::Node`:

| Method | Default | |
|---|---|---|
| `setHopTimeout(ms)` | 200 | Per-hop ACK wait. Must cover a frame and its ACK on air |
| `setHopRetries(n)` | 3 | Per-hop retries |
| `setAckTimeout(ms)` | 3000 | End-to-end ACK wait. Must cover every hop there and back |
| `setRouteTimeout(ms)` | 0 | Keep looking for a route this long. 0 is one attempt (RadioHead's fixed 4 s) |

**Long range.** Slower modem settings reach further but take much longer on air: at SF12 (`RH_RF95::Bw125Cr48Sf4096`) one short message takes about 3 s, so every timeout has to grow with it. The `MeshNode` sketch's `longrange` settings (in this repo's `platformio.ini`) are a starting point: hop 2.5 s, ACK 30 s, route 30 s. They're calculated from time on air, not measured yet, so tune them on your hardware, and use `RangeTest` to compare modem settings first.

**Message size.** `Node::maxPayload()` is 243 bytes on the RFM95 (251 bytes per frame, minus the mesh headers).

## Hardware

ESP32 boards with an RFM95 LoRa module, at least two. Check that the frequency matches your module and your region (`RF95_FREQ`, default 915 MHz).

```
[RFM95] ------------- [ESP32]
RESET  -------------- GPIO14
NSS/CS -------------- GPIO5
SCK    -------------- GPIO18
MOSI   -------------- GPIO23
MISO   -------------- GPIO19
DIO0   -------------- GPIO2

3.3V   -------------- 3.3V
GND    -------------- GND
```
<details>
<summary>Schematic</summary>
<img src="docs/esp32-pinout.jpeg" width="700">
<br/>
<img src="docs/wiring-schematic.jpeg" width="500">

</details>

These are the defaults in [`rhmesh/Board.h`](src/rhmesh/Board.h). For other wiring, define `RFM95_CS`, `RFM95_RST` and `RFM95_INT` before including the library, or as build flags. The TTGO T-Beam uses CS 18, RST 14, INT 26.

## Developing this library

This repository is the library and its development project at once. [`platformio.ini`](platformio.ini) builds each example against the library in the repo (`lib_deps = symlink://.`):

```bash
pio test -e native
```

```bash
pio run -e sim-demo -t exec
```

```bash
pio run -e node-id-1 -e node-id-254 -t upload
```

- `node-id-1/3/254`: the `MeshNode` sketch on a T-Beam (1) and ESP32 DOIT boards (3, 254). Set `monitor_port`/`upload_port` to your COM ports.
- `testnet-node-1/2/3`: forces the line `1 - 2 - 3` on a desk with `RH_TEST_NETWORK=4`, which makes RadioHead drop frames that skip node 2. It has to be a build flag, because RadioHead reads it when `RHRouter.cpp` is compiled.
- `longrange-node-1/254`: SF12 with the scaled timeouts.
- `range-test-*`, `static-routing-node-*`: the other two examples.

CI runs the simulator tests, builds every env, compiles the sketches with the Arduino IDE toolchain, and builds a separate project that depends on the library. Known issues are in [`BUGS.md`](BUGS.md).

```
src/LoRaRHMesh.h          Include this
src/rhmesh/Node.*         Mesh node: end-to-end ACK, route timeout, bounded send/receive
src/rhmesh/Board.h        Default pins, frequency and radio reset (Arduino only)
src/rhmesh/sim/           Simulator (host builds only)
extras/sim-radiohead/     RadioHead mesh code for the simulator, see PATCHES.md
extra_script.py           Puts extras/sim-radiohead on the include path in native builds
examples/                 Arduino sketches
test/test_mesh/           Simulator tests
```

## License

GPL-3.0, because RadioHead is licensed under GPL v3 (or commercially by its author). See [`LICENSE`](LICENSE).
