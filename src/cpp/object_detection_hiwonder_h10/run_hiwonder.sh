#!/usr/bin/env bash
# Reto 2 (obstacle challenge) con la placa Hiwonder. Arranque genérico: el
# build y el modelo los fija el script del robot (run_hiwonder_h8.sh) o las
# variables WRO_HIWONDER_BUILD_DIR y WRO_HIWONDER_MODEL.
#
#   ./run_hiwonder_h8.sh                     # reto completo: espera el botón
#   ./run_hiwonder_h8.sh --solo-percepcion   # cámara + Hailo + lidar, sin abrir la placa
#   ./run_hiwonder_h8.sh --no-display        # sin ventanas (por SSH o como servicio)
#   WRO_MAX_RPS=1.5 ./run_hiwonder_h8.sh     # primeras pruebas, despacio
set -euo pipefail

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${WRO_HIWONDER_BUILD_DIR:-$source_dir/build/h8_s2l}"
model="${WRO_HIWONDER_MODEL:-$source_dir/../../HailoModels/wro_h8_yolov8n_v4.hef}"

solo_percepcion=0
args=()
for arg in "$@"; do
    if [[ "$arg" == "--solo-percepcion" ]]; then
        solo_percepcion=1
    else
        args+=("$arg")
    fi
done

if [[ ! -x "$build_dir/object_detection" ]]; then
    echo "No existe $build_dir/object_detection" >&2
    echo "Compile primero: cmake -S '$source_dir' -B '$build_dir' -DCMAKE_BUILD_TYPE=Release && make -C '$build_dir' -j\$(nproc)" >&2
    exit 1
fi
if [[ ! -f "$model" ]]; then
    echo "No existe el modelo $model" >&2
    exit 1
fi

if [[ "$solo_percepcion" == 1 ]]; then
    echo "Solo percepción: cámara + Hailo + lidar; la placa Hiwonder no se abre."
else
    echo "Reto 2: al arrancar centra la dirección y calibra el giroscopio 5 s (robot QUIETO);"
    echo "el movimiento espera el botón. Ctrl+C = parada de emergencia."
fi

# Los .yaml de configuración están junto al ejecutable; sudo porque el reto 1
# también se lanza así (lidar, Hailo y GPIO).
cd "$build_dir"
export WRO_SOLO_PERCEPCION="$solo_percepcion"
exec sudo -E ./object_detection \
    --net "$model" \
    --input rpi \
    "${args[@]}"
