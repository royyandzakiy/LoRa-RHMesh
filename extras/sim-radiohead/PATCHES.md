# RadioHead for the host simulator

The mesh modules of RadioHead 1.143.1 (from the Arduino Library Manager release, mirrored at
[epsilonrt/RadioHead](https://github.com/epsilonrt/RadioHead), GPL v3, see `LICENSE`), compiled
only by native builds through `src/rhmesh/sim/sim_radiohead.cpp`. Boards use the normal
RadioHead dependency, unpatched.

Only `RadioHead.h`, `RHutil/simulator.h` and the generic driver, datagram, reliable datagram,
router and mesh classes are copied. The rest of RadioHead needs radio hardware.

## Changes from upstream

### Per-instance message buffers
`RHRouter::_tmpMessage` (`RHRouter.h`, `RHRouter.cpp`) and `RHMesh::_tmpMessage`
(`RHMesh.h`, `RHMesh.cpp`) changed from `static` members to normal members.

Upstream shares one buffer between all instances, so several mesh nodes in one process
corrupt each other's messages, for example a hop retry resending another node's frame.
A board has one instance, so it isn't affected.

### `_promiscuous` initialised to false
`RHGenericDriver`'s constructor (`RHGenericDriver.cpp`) left `_promiscuous` uninitialised.
A driver in static storage is zeroed, but a simulated driver lives on the heap or stack and
could start promiscuous. RHRouter then forwards frames it only overheard, which breaks
routing. `rhmesh::Node::init()` also sets it, so boards are covered without this patch.

### Windows hosts use the UNIX platform
`RadioHead.h` maps `_WIN32` to `RH_PLATFORM_UNIX`, so the simulator builds with MinGW g++
without extra flags. Upstream only detects Linux and macOS.

## Added files
- `netinet/in.h`: RadioHead.h includes it on its UNIX platform. Empty on Windows (MinGW),
  the real header elsewhere.
