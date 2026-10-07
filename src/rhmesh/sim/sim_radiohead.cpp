// Host simulator only. Boards compile this file empty.
#ifndef ARDUINO

// Only the parts of RadioHead the mesh needs, from extras/sim-radiohead (patched, see
// PATCHES.md there). extra_script.py puts that folder on the include path for native builds.
#include <RHGenericDriver.cpp>
#include <RHDatagram.cpp>
#include <RHReliableDatagram.cpp>
#include <RHRouter.cpp>
#include <RHMesh.cpp>

#endif  // ARDUINO
