#!/bin/bash
# One-time setup of the build environment in WSL (Ubuntu 24.04) and build of
# the Obstacle Challenge program for the twin.
#   bash wsl_setup.sh [repo_dir]
set -e
SIM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="${1:-$HOME/wro_repo}"
if ! pkg-config --exists opencv4 yaml-cpp libzmq libcurl 2>/dev/null; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake pkg-config libopencv-dev libyaml-cpp-dev libzmq3-dev libcurl4-openssl-dev
fi
if [ ! -d "$REPO/src/cpp/object_detection_hiwonder_h10" ]; then
    git clone --depth 1 https://github.com/alex309-duarte/WRO_Future_Engineers_Queretaro_CSS.git "$REPO"
fi
cmake -S "$SIM_DIR" -B "$HOME/wro_twin_build" -DWRO_REPO="$REPO" -DCMAKE_BUILD_TYPE=Release
cmake --build "$HOME/wro_twin_build" -j"$(nproc)"
echo "Built: $HOME/wro_twin_build/object_detection_twin (mission as in the repository)"
# Second build: the mission with the proposed change for the middle sign
cmake -S "$SIM_DIR" -B "$HOME/wro_twin_build_adapted" -DWRO_REPO="$REPO" -DCMAKE_BUILD_TYPE=Release \
      -DWRO_MISSION_PATCH="$SIM_DIR/proposed/mission_changes.patch"
cmake --build "$HOME/wro_twin_build_adapted" -j"$(nproc)"
echo "Built: $HOME/wro_twin_build_adapted/object_detection_twin (mission + proposed/mission_changes.patch)"
