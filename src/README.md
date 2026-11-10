# Source code (`src/`)

Software of ***"The Maker"***, design v7 (WRO 2026 Future Engineers International Final). Everything runs in C/C++ on a Raspberry Pi 5. The robot has **two programs**, one per challenge, and both use the same hardware layer for the Hiwonder ROS Robot Controller.

The full explanation (flowcharts, state machines, thresholds, testing) is in sections 6 and 9 of the [main README](../README.md#software).

## Where each program is

| Program | Folder | Hardware it uses | Build and run |
|---|---|---|---|
| Open Challenge | [`ondevice/`](ondevice) (`main.cpp`) | RPLidar S2L, Hiwonder board (motor, steering servo, IMU), start button | [README §9.5](../README.md#setup-instructions) |
| Obstacle Challenge | [`cpp/object_detection_hiwonder_h10/`](cpp/object_detection_hiwonder_h10) (`object_detection.cpp`) | Camera Module 3 Wide, Hailo-8, RPLidar S2L, Hiwonder board, start button | [README §9.6](../README.md#setup-instructions), [folder README](cpp/object_detection_hiwonder_h10/README.md) |

## Folders

| Folder | Contents |
|---|---|
| [`ondevice/`](ondevice) | Open Challenge mission and the shared hardware layer: LiDAR driver (`Oradar_S2L.cpp`), SPIKE-compatibility layer (`spike_compat.cpp`, `spike_hiwonder.cpp`), board protocol (`hiwonder/`), emergency stop (`parada_emergencia.cpp`), GPIO (`rasp_gpio.cpp`), calibration (`hiwonder_config.h`), offline tests (`tests/`). See [`ondevice/HIWONDER.md`](ondevice/HIWONDER.md). |
| [`cpp/object_detection_hiwonder_h10/`](cpp/object_detection_hiwonder_h10) | Obstacle Challenge mission of design v7: vision pipeline, LiDAR wall detection, avoidance and parking, offline tests. |
| [`cpp/common/`](cpp/common) | Hailo inference helpers used by the vision pipeline. |
| [`cpp/external/`](cpp/external) | Third-party libraries (`yaml-cpp`, `curl`). |
| [`cpp/object_detection_original_h10/`](cpp/object_detection_original_h10) | Obstacle Challenge program of design v6 (National Final: SPIKE hub, Oradar MS200, Hailo-10H), kept for reference. Its documentation is in [that folder's README](cpp/object_detection_original_h10/README.md). |
| [`HailoModels/`](HailoModels) | Compiled detection models (`.hef`). Design v7 uses `wro_h8_yolov8n_v4.hef`. |
| [`config/`](config) | Configuration files of the Hailo example framework. |
| [`tools/`](tools) | Helper scripts of the Hailo example framework. |
| `ondevice/rplidar_sdk/` | Slamtec RPLIDAR SDK, a Git submodule (`git clone --recurse-submodules`, or `git submodule update --init`). |
| `ondevice/oradar_sdk/` | Oradar SDK, used only by the design v6 build. |

## Quick start

```bash
git clone --recurse-submodules https://github.com/alex309-duarte/WRO_Future_Engineers_Queretaro_CSS.git
cd WRO_Future_Engineers_Queretaro_CSS/src/ondevice
(cd rplidar_sdk && make)                 # Slamtec SDK, once
make RPLIDAR_SDK_DIR=$PWD/rplidar_sdk    # Open Challenge program
make test                                # 53 offline checks, no hardware needed
```

The Obstacle Challenge build, the board firmware and the first-run checks are in [README §9](../README.md#setup-instructions).

Other documents in this folder: [`WALL_DETECTION.md`](WALL_DETECTION.md) (how the LiDAR wall detection works).
