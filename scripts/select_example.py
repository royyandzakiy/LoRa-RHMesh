# Builds examples/<custom_example>/ as the env's source dir. PlatformIO only converts .ino
# files at the top of src_dir, so a source filter into examples/ can't build the sketches.
Import("env")

import os

example = env.GetProjectOption("custom_example", "")
if example:
    env.Replace(PROJECT_SRC_DIR=os.path.join(env.subst("$PROJECT_DIR"), "examples", example))
