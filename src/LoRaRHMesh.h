#pragma once

// LoRa-RHMesh: a LoRa mesh node with end-to-end ACKs on top of RadioHead's RHMesh,
// plus a host simulator for testing mesh code without radios.

#include "rhmesh/Node.h"

#ifdef ARDUINO
#include "rhmesh/Board.h"
#else
#include "rhmesh/sim/SimEther.h"
#include "rhmesh/sim/SimNode.h"
#endif
