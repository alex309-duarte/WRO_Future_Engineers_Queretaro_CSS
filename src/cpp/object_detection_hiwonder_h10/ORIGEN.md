# Provenance of the Obstacle Challenge with Hiwonder

Rewritten on 2026-10-18 on the robot's Raspberry Pi 5 (Hailo-8, RPLidar S2L,
Hiwonder board with firmware `pos-v8`), on branch `reto1-hiwonder`, which was
merged into `main` on 2026-11-08.

## Mission

`object_detection.cpp` is the `object_detection.cpp` from
`../object_detection_original_h10` as it is in the working tree
(commit `e2bf80f` "WRO National competition" + the local changes in that
folder), that is, the complete mission from the national competition: laps in
both directions, corner, middle cube and parking. It is generated with targeted
substitutions on that file (each change carries the comment `Hiwonder:`), so
`diff --strip-trailing-cr` against the original shows only what changes: board
startup, shutdown, the lidar-monitored advance and three waits that had no exit
condition. The original is not modified.

## Hiwonder layer

There is no copy of the library in this folder. The Open Challenge sources are
compiled directly, the same ones that ran on the track on 2026-10-18:

| Source | What it provides |
|---|---|
| `../../ondevice/hiwonder/` | `hiwonder_brick` (board protocol) and `spike_compat` (SPIKE API) |
| `../../ondevice/spike_hiwonder.cpp` | `Spike_Hiwonder_Init`: servo, centring, gyroscope calibration |
| `../../ondevice/parada_emergencia.cpp` | Ctrl+C / kill / crash = motors in COAST and servo torque off |
| `../../ondevice/hiwonder_config.h` | **All of the robot's calibration** |
| `../../ondevice/Oradar_S2L.cpp` | RPLidar S2L driver (no quality filter) and 360° buffer |

The following were added to `spike_compat`, without changing what the Open
Challenge uses: `Spike_Advance_For_Degrees` with reverse (`speed < 0`, like
`ag_u()` on the hub), `Advance_For_Degrees_Until` (advance with a stop
condition), `Steer_Tire_Turn` and `Spike_Update_Config`. The fake board used in
the Open Challenge tests gained `cancel_motor_position`. The 53 Open Challenge
checks still pass.

## Model

`../../HailoModels/wro_h8_yolov8n_v4.hef`: YOLOv8n trained on 2026-10-18 with
the complete Roboflow dataset (`wro-dataset-completo`, 9354 images with x3
augmentation) and compiled with Dataflow Compiler 3.34 for Hailo-8. Classes in
the order `greenbox, redbox, xparking` (class_id 1, 2, 3 in `parse_nms_data`).
Evaluated on this same chip with the 477 October validation images:
mAP50 0.990 and correct round-2 decision in 99.2 % of cases (8 ms per image).
`yolov8n_wro_hailo8.hef` (the previous model) remains in the folder for
comparison: `WRO_HIWONDER_MODEL=... ./run_hiwonder_h8.sh`.
