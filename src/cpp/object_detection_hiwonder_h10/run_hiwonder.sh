#!/usr/bin/env bash
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h10_hiwonder}"
model="${WRO_HIWONDER_MODEL:-/home/maker/WRO_Hailo10H_Compatible/hailo10h_runtime/models/roboflow_yolov8n_wro_h10.hef}"
env_file="${WRO_HIWONDER_ENV:-$source_dir/hiwonder.env}"

arm=0
if [[ "${1:-}" == "--arm" ]]; then
    arm=1
    shift
fi

if [[ -f "$env_file" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "$env_file"
    set +a
fi

export HIWONDER_ARM="$arm"

if [[ ! -x "$build_dir/object_detection" ]]; then
    echo "No existe $build_dir/object_detection" >&2
    echo "Compile primero con cmake --build '$build_dir' -j\$(nproc)" >&2
    exit 1
fi

if [[ "$arm" == 0 ]]; then
    echo "Arranque seguro: Hailo + cámara + LiDAR; Hiwonder desarmado."
else
    echo "Arranque ARMADO solicitado. El movimiento aún espera el botón físico."
fi

exec sudo -E "$build_dir/object_detection" \
    --net "$model" \
    --input rpi \
    "$@"
