# Local patches to RadioHead 1.120

This copy is vendored on purpose. Changes from upstream:

## Per-instance message buffers
`RHRouter::_tmpMessage` (`RHRouter.h`, `RHRouter.cpp`) and `RHMesh::_tmpMessage`
(`RHMesh.h`, `RHMesh.cpp`) changed from `static` members to normal members.

Upstream shares one buffer between all instances, so several mesh nodes in one
process (the host simulator) corrupt each other's messages. On a device there is
only one instance, so RAM use is unchanged.
