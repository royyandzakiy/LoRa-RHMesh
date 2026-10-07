# Local patches to RadioHead 1.120

This copy is vendored on purpose. Changes from upstream:

## Per-instance message buffers
`RHRouter::_tmpMessage` (`RHRouter.h`, `RHRouter.cpp`) and `RHMesh::_tmpMessage`
(`RHMesh.h`, `RHMesh.cpp`) changed from `static` members to normal members.

Upstream shares one buffer between all instances, so several mesh nodes in one
process (the host simulator) corrupt each other's messages. On a device there is
only one instance, so RAM use is unchanged.

## `_promiscuous` initialised to false
`RHGenericDriver`'s constructor (`RHGenericDriver.cpp`) left `_promiscuous`
uninitialised. A driver in static storage is zeroed, so this only shows up for a
driver on the stack or heap: it may start promiscuous, and RHRouter then forwards
frames it only overheard, which breaks routing.
