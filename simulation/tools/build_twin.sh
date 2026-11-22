#!/bin/bash
# Builds the two twin programs in WSL / Linux:
#   ~/wro_twin_build          mission exactly as in the repository
#   ~/wro_twin_build_adapted  mission + proposed/mission_changes.patch
# Usage: bash build_twin.sh [path to the repository clone, default ~/wro_repo]
set -e
SIM_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO="${1:-$HOME/wro_repo}"
cmake -S "$SIM_DIR" -B "$HOME/wro_twin_build" -DWRO_REPO="$REPO" -DCMAKE_BUILD_TYPE=Release | grep -i "twin" || true
cmake --build "$HOME/wro_twin_build" -j"$(nproc)" | tail -1
cmake -S "$SIM_DIR" -B "$HOME/wro_twin_build_adapted" -DWRO_REPO="$REPO" -DCMAKE_BUILD_TYPE=Release \
      -DWRO_MISSION_PATCH="$SIM_DIR/proposed/mission_changes.patch" | grep -i "twin" || true
cmake --build "$HOME/wro_twin_build_adapted" -j"$(nproc)" | tail -1
ls -la "$HOME/wro_twin_build/object_detection_twin" "$HOME/wro_twin_build_adapted/object_detection_twin"
