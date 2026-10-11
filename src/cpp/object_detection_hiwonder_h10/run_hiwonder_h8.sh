#!/usr/bin/env bash
# Robot con Hailo-8 (26 TOPS) y RPLidar S2L: mismo arranque que
# run_hiwonder.sh, con el build h8_s2l y el modelo compilado para Hailo-8.
# Admite los mismos argumentos (--arm, --no-display...).
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export WRO_HIWONDER_BUILD_DIR="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h8_s2l}"
export WRO_HIWONDER_MODEL="${WRO_HIWONDER_MODEL:-$source_dir/../../HailoModels/yolov8n_wro_hailo8.hef}"
exec "$source_dir/run_hiwonder.sh" "$@"
