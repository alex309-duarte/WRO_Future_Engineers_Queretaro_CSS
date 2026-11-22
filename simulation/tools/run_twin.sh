#!/bin/bash
# Runs the Obstacle Challenge program of the repository against the CoppeliaSim
# scene (the simulation must be running). Extra arguments go to the program,
# e.g. --no-display, -s -o <dir> (save the processed camera video).
#   WRO_SIM_AUTOSTART=3 bash run_twin.sh       press the start button after 3 s
BUILD="${WRO_TWIN_BUILD:-$HOME/wro_twin_build}"
cd "$BUILD" || exit 1
export WRO_RUMBO_LOG="${WRO_RUMBO_LOG:-$BUILD/rumbo_log_twin_$(date +%Y%m%d_%H%M%S).csv}"
exec stdbuf -oL -eL ./object_detection_twin --framerate 15 "$@"
