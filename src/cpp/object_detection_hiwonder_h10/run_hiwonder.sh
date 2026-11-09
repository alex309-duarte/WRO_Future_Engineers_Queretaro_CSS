#!/usr/bin/env bash
# Obstacle Challenge (reto 2) launcher for the robot with the Hiwonder
# controller board. Generic start script: the build directory and the Hailo
# model are set by the robot-specific script (run_hiwonder_h8.sh) or by the
# variables WRO_HIWONDER_BUILD_DIR and WRO_HIWONDER_MODEL (defaults below:
# build/h8_s2l and the YOLOv8n v4 model for Hailo-8).
#
#   ./run_hiwonder_h8.sh                     # full run: waits for the start button
#   ./run_hiwonder_h8.sh --solo-percepcion   # camera + Hailo + LiDAR only, board not opened
#   ./run_hiwonder_h8.sh --no-display        # no windows (over SSH or as a service)
#   WRO_MAX_RPS=1.5 ./run_hiwonder_h8.sh     # first tests, slow (speed cap in rev/s)
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h8_s2l}"
model="${WRO_HIWONDER_MODEL:-$source_dir/../../HailoModels/wro_h8_yolov8n_v4.hef}"

# --solo-percepcion ("perception only") is consumed here and passed to the
# program as WRO_SOLO_PERCEPCION; every other argument is forwarded unchanged.
solo_percepcion=0
args=()
for arg in "$@"; do
    if [[ "$arg" == "--solo-percepcion" ]]; then
        solo_percepcion=1
    else
        args+=("$arg")
    fi
done

# Fail early with a hint if the executable or the model file is missing.
if [[ ! -x "$build_dir/object_detection" ]]; then
    echo "$build_dir/object_detection does not exist" >&2
    echo "Build it first: cmake -S '$source_dir' -B '$build_dir' -DCMAKE_BUILD_TYPE=Release && make -C '$build_dir' -j\$(nproc)" >&2
    exit 1
fi
if [[ ! -f "$model" ]]; then
    echo "Model file $model does not exist" >&2
    exit 1
fi

if [[ "$solo_percepcion" == 1 ]]; then
    echo "Perception only: camera + Hailo + lidar; the Hiwonder board is not opened."
else
    echo "Obstacle Challenge: at startup it centres the steering and calibrates the gyroscope for 5 s (keep the robot STILL);"
    echo "motion waits for the button. Ctrl+C = emergency stop."
fi

# The configuration .yaml files are copied next to the executable, so run from
# the build directory; sudo because the Open Challenge is launched the same
# way (LiDAR, Hailo and GPIO access). -E keeps the WRO_* variables.
cd "$build_dir"
export WRO_SOLO_PERCEPCION="$solo_percepcion"
exec sudo -E ./object_detection \
    --net "$model" \
    --input rpi \
    "${args[@]}"
