# Changelog

## 1.0.0

First release as a library. Before this, the repository was a PlatformIO project;
the original thesis code is in the `v0-thesis` tag.

- `rhmesh::Node`: RHMesh with end-to-end ACKs, bounded send and receive, payload limit
  from the radio driver, and refusal of the broadcast address.
- `Node::setRouteTimeout()`: keeps looking for a route past RadioHead's fixed 4 s, needed
  for multi-hop at SF12.
- Simulator for host builds: `rhmesh::SimEther` (radio channel with links, loss, power
  and frame trace) and `rhmesh::SimNode` (one board per thread).
- Examples: `MeshNode`, `RangeTest`, `StaticRouting`.
- Depends on RadioHead 1.143.1, which builds on Arduino-ESP32 core 3.
- License: GPL-3.0.
