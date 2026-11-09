#!/usr/bin/env bash
# Obstacle Challenge launcher for the robot with the Hailo-8 (26 TOPS) and the
# RPLidar S2L: selects the h8_s2l build and the YOLOv8n v4 model compiled for
# Hailo-8 (classes greenbox, redbox, xparking = green/red traffic signs and
# magenta parking-lot markers), then runs run_hiwonder.sh. Accepts the same
# arguments as run_hiwonder.sh (--solo-percepcion, --no-display...).
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export WRO_HIWONDER_BUILD_DIR="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h8_s2l}"
export WRO_HIWONDER_MODEL="${WRO_HIWONDER_MODEL:-$source_dir/../../HailoModels/wro_h8_yolov8n_v4.hef}"
exec "$source_dir/run_hiwonder.sh" "$@"
