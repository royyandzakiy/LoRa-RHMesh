# Revival plan

Turning this thesis snapshot into a working reference repo, in 4 PRs. Status: all 4 PRs done, each branch stacked on the previous one: `feat/meshnode-core` → `feat/native-sim` → `feat/examples-ci` → `feat/docs`. Left to do: push the `v0-thesis` tag, merge in order, and test the `longrange-*` timeouts on hardware.

## Context
The repo has stars and forks but is a frozen thesis snapshot. People use it as a small, readable RHMesh-on-ESP32 example. The goal is to keep it small but make it **correct, runnable without hardware, and checked by CI**:
- `src/main.cpp` has real bugs. It runs `sprintf("%s")` on a non-null-terminated RX buffer, and `_msgRcvBufLen` is never reset before the second `recvfromAckTimeout`.
- The two `BUGS.md` items (`sendtoWait` returns `NONE` with no peers, random stalls) are still open.
- The README tells users to put `#define RH_TEST_NETWORK` in `main.cpp`. That has **no effect**, because the macro is read in `lib/RadioHead/RHRouter.cpp:223`, a separate translation unit, so it must be a build flag.
- There's no way to try the mesh without 2–4 boards, and there's no CI.

Decisions made: own native simulator, tag then curate the old sketches, keep vendored RadioHead 1.120.

Key finding: `RHRouter::_tmpMessage` is a **static** class member (`RHRouter.h:331`, `RHRouter.cpp:16`). Multiple mesh instances in one process would share it, so the in-process simulator needs a small patch to the vendored lib. That is also a good reason to keep it vendored.

## Work, as 4 PRs in order

### PR 1: Tag, clean up, extract a portable node core
1. `git tag v0-thesis` on the current master and push it. The README links to it as "original thesis code".
2. Patch the vendored lib: change `static RoutedMessage _tmpMessage` to a non-static member (`RHRouter.h:331`, remove the definition at `RHRouter.cpp:16`). RAM use on a device stays the same (one instance). Record it in a new `lib/RadioHead/PATCHES.md`.
3. New local library `lib/MeshNode/` (`MeshNode.h/.cpp`), with no Arduino/ESP32 dependencies, only RHMesh:
   - Small app header `{uint8_t type; uint8_t seq;}`, with types `DATA` and `ACK`. This gives the **end-to-end delivery confirmation** the README says is missing.
   - `send(dest, buf, len)` → returns a status covering local hop failure, sent awaiting E2E ACK, delivered, and E2E timeout.
   - `poll(timeoutMs)`: receives once. A DATA message → `onMessage(from, buf, len, rssi)` callback, then an automatic ACK. An ACK → match the pending seq → `onDelivered(dest, seq)`.
   - Explicit lengths everywhere. No `sprintf` on payloads, no reliance on null terminators.
   - Expose the route table (wrap `RHRouter::getRouteTo`/`printRoutingTable`) for tests and debugging.
   - Configurable `setTimeout`/`setRetries` passed through to RHReliableDatagram, so `sendtoWait` time is bounded.
4. Rewrite `src/main.cpp` as a thin ESP32 shell: radio setup (`rhSetup()` stays), watchdog (keep the `esp_task_wdt` pattern), and serial logging with `%.*s`. The send/receive state machine moves into `MeshNode`.
5. `platformio.ini`: add `-D RH_TEST_NETWORK=N` as an optional env (e.g. `[env:node-id-1-testnet1]`) so forced topology really works on hardware.

### PR 2: Native simulator and tests
- `[env:native]` in `platformio.ini`: `platform = native`, `lib_ignore = RadioHead` (the full vendored lib won't compile on host because of SPI and radio drivers), `build_flags = -I lib/RadioHead -pthread`.
- `sim/` folder (added via `test_build_src`/`build_src_filter`):
  - `rh_sim_unity.cpp`: `#include`s only `RHGenericDriver.cpp, RHDatagram.cpp, RHReliableDatagram.cpp, RHRouter.cpp, RHMesh.cpp`.
  - `sim_platform.cpp`: implements the externs from `RHutil/simulator.h` (`millis`, `delay`, `random`, `Serial`, `_simulator_argc/argv`).
  - `SimDriver : RHGenericDriver`: implements `available/recv/send/maxMessageLength`, with a fake `lastRssi`.
  - `Ether`: shared, mutex-protected. It delivers a frame to every node whose link to the sender is up in a **link matrix**. It also supports per-link drop rate and turning a node off. This replaces the hardcoded `RH_TEST_NETWORK` topologies with data.
  - One `std::thread` per node, real time (fine for RHMesh timeouts of a few seconds).
- `test/test_mesh/` (PlatformIO Unity), scenarios:
  - Line 1-2-3-4: 1→4 delivered, E2E ACK received, node 1's route to 4 goes via 2.
  - Diamond (old `RH_TEST_NETWORK` 3): delivery survives killing one middle node after a route rebuild.
  - No peers: reproduce `BUGS.md` #1 and assert the correct error. Spike, timebox ½ day: find out whether `RH_ROUTER_ERROR_NONE` comes from a stale route or ARP behaviour, then fix it in `MeshNode` (preferred) or patch the vendored lib and log it in `PATCHES.md`.
  - Payload at `RH_MESH_MAX_MESSAGE_LEN` and one byte over (rejected); a non-null-terminated payload round-trips.
- `sim/demo.cpp`, an optional runnable program (`pio run -e sim-demo -t exec`): 4 nodes chatting with the same log output as the firmware. This is what the README points newcomers to.
- Platform note: RadioHead's UNIX platform needs `<netinet/in.h>`, so native runs on Linux, macOS or WSL, not plain Windows. Document it.

### PR 3: Curated examples and CI
- Delete `projects/dump/` (kept in the `v0-thesis` tag).
- Port `projects/test-1..4` to `examples/01-broadcast-range`, `02-routing`, `03-mesh`, `04-mesh-long-range`. Each is a single `.cpp` built with `build_src_filter = +<../examples/NN-*/>` and reuses the existing `[board-esp32doit]`/`[board-tbeam]` pin sections. Remove the copy-pasted pin `#if` blocks and use the `-D RFM95_*` flags like `src/main.cpp` does now.
- `.github/workflows/ci.yml` (ubuntu, PlatformIO cache): `pio run` for every hardware env and example (compile only), and `pio test -e native`.

### PR 4: Docs
- Rewrite the README around three entry points: **try it in the simulator (no hardware)**, **flash two boards**, **understand how RHMesh routes**. Keep the existing wiring, topology images and the "how send/receive works" section, corrected:
  - `RH_TEST_NETWORK` must be a build flag.
  - E2E ACK now exists.
  - Fix the "send every 10 seconds" comment vs `3000`.
- Update `BUGS.md` to list status and the sim test that covers each bug. The stall bug stays open unless the sim reproduces it.
- Add a CI badge and a short "Original thesis code → tag `v0-thesis`" note.

### Found while doing PR 1
- **BUGS.md #1 root cause:** the end node used address 255, which is `RH_BROADCAST_ADDRESS`. RHMesh skips route discovery for it and RHReliableDatagram doesn't wait for an ACK, so `sendtoWait` returns `NONE` even with nobody listening, and nodes never routed through each other. The end node is now 254, and `main.cpp` refuses 255 at compile time.
- `RHMesh::_tmpMessage` is also static, not only `RHRouter`'s. Both are patched.
- Unpinned `platform = espressif32` now pulls Arduino-ESP32 3.x, which doesn't compile RadioHead (`RH_ASK` timers). It's pinned to `espressif32@6.10.0`.
- `MeshNode::send()` blocks until the end-to-end ACK or timeout (like the old send-then-wait-reply loop), instead of the async `onDelivered` callback. That's simpler to read, and an async version can come later if needed.
- `RH_TEST_NETWORK` is wired up as the 3-board `testnet-node-1..3` envs (topology 4 = line 1-2-3).

### Found while doing PR 2
- **Upstream RadioHead bug:** `RHGenericDriver`'s constructor never initializes `_promiscuous`. A global driver (the ESP32 case) is zeroed, so it's hidden there. A driver on the stack or heap can start promiscuous, and RHRouter then re-forwards frames it only overheard, which breaks routing. Patched in the vendored lib and logged in `PATCHES.md`.
- **Max payload:** `MeshNode::kMaxPayload` assumed RadioHead's generic 255-byte limit. RH_RF95 carries 251, so a full-size message would fail on hardware. Added `MeshNode::maxPayload()`, based on the driver (243 bytes on RFM95). The sim driver uses the same 251.
- The sim lives in `lib/MeshSim/` (a local library, native only) instead of `sim/`, with `sim/demo.cpp` as the runnable program. `SimEther::setTrace()` prints every frame on the air.
- It runs natively on Windows with MSYS2 g++: a stub `netinet/in.h` and `-D RH_PLATFORM=6` replace the Linux-only platform detection. Linux/WSL builds use the real header via `#include_next`.
- Unity aborts failed tests with `longjmp`, which skips destructors and leaves node threads running on freed memory. The `native` env sets `UNITY_EXCLUDE_SETJMP_H`.
- The tests pass with or without the `_tmpMessage` patch. The race is real but the window is too small to hit; the patch stays as a correctness fix.
- Two PlatformIO cores on one machine (`pio` on PATH 6.1.18, VS Code's 6.2.0) keep swapping SCons versions, and `-t exec` fails under the old one. Use one core.

### Found while doing PR 3
- **Long range never worked multi-hop:** `RH_MESH_ARP_TIMEOUT` is a fixed 4 s (with an upstream `FIXME`). At SF12 one frame takes about 3 s on air, so route discovery over 2 hops always timed out with "no route". It's now `#ifndef`-wrapped (in `PATCHES.md`) and raised by the `longrange-*` envs, together with the hop/ACK/watchdog timeouts.
- Long range is a set of build flags on `src/main.cpp` (`MODEM_CONFIG`, `HOP_TIMEOUT_MS`, `ACK_TIMEOUT_MS`, `SEND_INTERVAL_MS`, `WDT_TIMEOUT_S`), not a copy of it. The longrange timeouts are calculated, not measured on hardware yet.
- `test-2-mesh` is what `src/main.cpp` already does, so it has no example. `test-1` became `examples/02-static-routing` and `test-3` became `examples/01-range-test`. The four hand-written modem register sets match RadioHead's `ModemConfigChoice` presets, so the examples use those.
- Shared pins, frequency and radio reset live in `lib/BoardConfig/BoardConfig.h`. `platformio.ini` has a `pin_flags` set per board.
- CI builds every hardware env, reading the list from `platformio.ini`, plus the sim tests and demo.

## Critical files
- `src/main.cpp` (rewrite to a thin shell)
- `lib/MeshNode/*` (new)
- `lib/RadioHead/RHRouter.h:331`, `RHRouter.cpp:16` (patch), `lib/RadioHead/PATCHES.md` (new)
- `platformio.ini` (native, test-net and example envs)
- `sim/*`, `test/test_mesh/*` (new)
- `examples/*` (from `projects/test-*`)
- `.github/workflows/ci.yml`, `README.md`, `BUGS.md`

Reused as-is: the RadioHead simulator hooks (`RHutil/simulator.h`, `RH_PLATFORM_UNIX` path in `RadioHead.h:1685`), the existing board/env sections in `platformio.ini`, and `rhSetup()`.

## Verification
1. `pio test -e native` (WSL/Linux): all scenarios pass. Temporarily revert the `_tmpMessage` patch to confirm the multi-node tests fail without it, so they really test it.
2. `pio run -e sim-demo -t exec`: 4 nodes show routed delivery and E2E ACK logs.
3. `pio run` for every hardware env and example compiles cleanly.
4. Hardware smoke test (you, with 2–3 boards): flash `node-id-1` + `node-id-255`, see `delivered` with an E2E ACK in the serial monitor. Then flash a test-net env and confirm the forced route is used.
5. CI is green on the PR.

## Rough effort
PR1 ~1 day · PR2 ~1.5–2 days (including the ½-day BUGS #1 spike) · PR3 ~½–1 day · PR4 ~½ day.
