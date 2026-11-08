#!/usr/bin/env bash
# Robot con Hailo-8 (26 TOPS) y RPLidar S2L: build h8_s2l y modelo YOLOv8n v4
# compilado para Hailo-8 (clases greenbox, redbox, xparking). Admite los
# mismos argumentos que run_hiwonder.sh (--solo-percepcion, --no-display...).
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export WRO_HIWONDER_BUILD_DIR="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h8_s2l}"
export WRO_HIWONDER_MODEL="${WRO_HIWONDER_MODEL:-$source_dir/../../HailoModels/wro_h8_yolov8n_v4.hef}"
exec "$source_dir/run_hiwonder.sh" "$@"
