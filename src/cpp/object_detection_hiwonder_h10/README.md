# Obstacle Challenge with the Hiwonder board

The Obstacle Challenge mission from `../object_detection_original_h10` (the one
from the national competition, with parking) running on the Hiwonder board
instead of the LEGO SPIKE hub, with the **same layer the Open Challenge already
proved on the track** (`src/ondevice`, branch `reto1-hiwonder`). RPi camera,
Hailo-8, RPLidar S2L, GPIO4 button.

| Robot | Accelerator | Lidar | Build | Startup |
|---|---|---|---|---|
| Hailo-8 (this one) | Hailo-8 HAT, HailoRT 4.23 | RPLidar S2L | `build/h8_s2l` | `./run_hiwonder_h8.sh` |
| Oradar MS200 | any | Oradar MS200 | `cmake -DWRO_LIDAR=ORADAR` | `WRO_HIWONDER_BUILD_DIR=... ./run_hiwonder.sh` |

## What is in this folder

| File | What it is |
|---|---|
| `object_detection.cpp` | The original mission with Hiwonder board startup/shutdown (changes marked `Hiwonder:`) |
| `spike.h` | The mission's SPIKE API, on top of the Open Challenge `spike_compat` |
| `spike_reto2.cpp` | `Spike_Reto2_Init`, turns with `await`, advance in millimetres |
| `reto2_config.h` | What the Open Challenge does not need: **wheel diameter**, `tire_turn` scale |
| `lidar_reto2.cpp` | `Oradar_S2L_Advance_Until_Distance` with brake type |
| `rasp_gpio.cpp` | GPIO4 button and GPIO3 LED (gpiochip0 on the Pi 5), no relay |
| `tests/reto2_test.cpp` | 31 offline checks against the Open Challenge fake board |

**There is no copy of the Hiwonder library.** CMake compiles
`../../ondevice/hiwonder/`, `spike_hiwonder.cpp`, `parada_emergencia.cpp` and
`Oradar_S2L.cpp` from the Open Challenge directly, and all of the robot's
calibration (servo, heading PD, speed, emergency stop) is read from
`../../ondevice/hiwonder_config.h`: whatever is tuned there applies to both
challenges. See `ORIGEN.md`.

## Build and test without moving anything

```bash
cmake -S . -B build/h8_s2l -DCMAKE_BUILD_TYPE=Release
make -C build/h8_s2l -j"$(nproc)"
./build/h8_s2l/reto2_test            # 31 checks, 0 failures
(cd ../../ondevice && make test)     # the 53 Open Challenge checks still pass
./run_hiwonder_h8.sh --solo-percepcion            # camera + Hailo + lidar, the board is NOT opened
./run_hiwonder_h8.sh --solo-percepcion --no-display   # the same over SSH, no windows
```

`--solo-percepcion` opens the camera window (boxes with colour and confidence)
and the lidar window (detected front/right/left/rear walls). It is the way to
check the model and the lidar orientation before arming.

## Run the challenge

```bash
WRO_MAX_RPS=1.5 ./run_hiwonder_h8.sh   # first tests, slow
./run_hiwonder_h8.sh                   # competition speed (5.7 r/s)
./run_hiwonder_h8.sh --no-display      # over SSH / as a service
```

At startup: it opens the board, installs the emergency stop, checks that the
steering servo is within its travel, **centres it slowly** and **calibrates the
gyroscope for 5 s with the robot still**; then it brings up the camera, Hailo
and lidar, and the mission waits for the **button** (the LED blinks). The lap
direction is decided by the lidar when leaving the parking box
(`distancia_derecha > 600` = clockwise), as in the original.

**Ctrl+C stops the robot at any time** (so do `kill`, closing the terminal or a
crash): motors in COAST, servo torque off and process exit. It is the Open
Challenge `parada_emergencia`; it does not cover `kill -9` or a power loss on
the Pi (in that case: motor switch or cut the 12 V supply).

## Before the first run: calibrate

With the wheels lifted, in this order. The Open Challenge tools apply
(`~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/`).

1. **Open Challenge items** (yaw sign, steering side): already proven on the
   track; if `hiwonder_config.h` is changed, it changes for both challenges.
2. **Wheel diameter** (`kDiametroRuedaMm` in `reto2_config.h`, currently a
   nominal 65 mm). It is the only new quantity to measure: with it,
   `Spike_Advance_For_distance(…, mm, …)` covers real millimetres and the
   parking "hub degrees" (`Spike_Advance_For_Degrees(-50, 2250, …)`) are
   converted to the same distance the LEGO robot covered. Check:
   `Spike_Advance_For_distance(50, 1000, 0)` must cover 1000 mm; if it covers
   L mm, multiply the diameter by L/1000.
3. **Turns with partial `tire_turn`**. The mission turns with `tire_turn` 40
   (0.96 of full lock), 45 (full lock), 30 (0.72) and 15 (0.36):
   `Spike_Reto2_Init` scales the Open Challenge factor by the hub's 4.29° per
   `turn()` unit (`kGradosMotorPorTireTurn`). Turning radii change with the
   Ackermann linkage, so the avoidance angles (`abs(angle_to_wall) ± 5`) and the
   distances subtracted from the hypotenuse (`- 400`, `- 250`…) are tuned on the
   track, just as they were tuned with the LEGO robot.
4. **Reverse** (parking). `Spike_Advance_For_Degrees` and turns with
   `speed < 0` go backwards; when reversing, the heading correction inverts its
   sign and uses 0.15 of kp (like `ag_u()` on the hub, with no D term):
   `reverse_kp_factor` / `reverse_kd_factor` in `spike_compat::Config`. First
   test `Spike_Advance_For_Degrees(-50, 400, 0)` on a straight.

## Differences from the original (SPIKE)

- `Spike_Forward` and `Spike_Follow_Reference` execute one PD step per call;
  the lidar loops call them every 1 ms and the IMU arrives at 20 Hz (the D term
  only sees a change in 1 of every ~50 calls). If the steering oscillates on the
  straights, this is the reason.
- Advances go through the STM32 position loop (more accurate than the hub's
  `set_duty_cycle`) and have a timeout; `speed` is the speed limit.
- `Spike_Hold_Motors` uses the position HOLD of the `pos-v8` firmware;
  `Spike_Break_Motors` stops the drive (the servo already holds).
- `Advance_For_distance_Especial` (lidar-monitored advance): a 0 reading on the
  monitored ray no longer aborts the advance (the original treated it as
  "less than 370 mm").
- Three waits that could run indefinitely now have an exit: the cube wait in
  `estacionamiento_*` (it declared another variable inside; now it waits 3 s and
  continues), the "valid wall" wait when computing the avoidance angle, and the
  closing of the sweep in the visualisation (375° → 360°).
- No GPIO2 relay and no REPL: Ctrl+C before the board is opened shuts down the
  pipeline cleanly; after that it is the emergency stop.

## Verification performed (2026-10-18, on the robot)

- Builds on the Pi 5 (GCC 14.2, HailoRT 4.23, OpenCV 4.10) with only the
  warnings the original already had.
- `reto2_test`: 31 checks, 0 failures. Open Challenge `make test`: 51, 0 failures.
  The Open Challenge (`my_lidar_app`) still builds with the shared layer.
- `--solo-percepcion --no-display` for 20 s: RPLidar on `/dev/ttyUSB0` (front
  1848 mm, right 629, left 258), camera 800x600, Hailo-8 at 29.97 FPS,
  clean exit with Ctrl+C.
- Armed for 30 s: servo read at 854 (within the startup range 584..916),
  centred, gyroscope calibrated, `tire_turn 40 = 0.96`, lidar ready, pipeline
  up, waiting for the button; Ctrl+C → "Parada de emergencia: motores en COAST",
  process terminated and board port released.

Track runs started with the button (2026-11-08): see
`other/testing/WRO2026_v7_test_log.xlsx` and README section 6.9.
