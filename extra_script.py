# Native (host) builds only: compile the simulator against the patched RadioHead copy in
# extras/sim-radiohead. Board builds use the normal RadioHead dependency and skip this.
Import("env")

import os

if env.get("PIOPLATFORM") == "native":
    lib_dir = Dir(".").srcnode().abspath
    sim_radiohead = os.path.join(lib_dir, "extras", "sim-radiohead")
    print("LoRa-RHMesh: simulator RadioHead at", sim_radiohead)

    # The project's own sources and tests include the library headers too, so the
    # global environment needs the same paths as the library's.
    for e in (env, DefaultEnvironment()):
        e.Append(
            CPPPATH=[sim_radiohead],
            CCFLAGS=["-pthread"],
            LINKFLAGS=["-pthread"],
        )
