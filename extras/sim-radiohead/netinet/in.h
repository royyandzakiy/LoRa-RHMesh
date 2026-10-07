#pragma once

// RadioHead.h includes this on its UNIX platform. The mesh code doesn't use it,
// so on Windows (MinGW) an empty header lets the simulator build without WSL.
#if !defined(_WIN32)
#include_next <netinet/in.h>
#endif
