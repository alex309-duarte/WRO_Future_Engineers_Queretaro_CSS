# Open Challenge with the Hiwonder board

On the `reto1-hiwonder` branch, the Open Challenge in `main.cpp` runs on the
Hiwonder board (STM32, firmware `pos-v8`) instead of the SPIKE hub, without a
relay. The challenge logic, the lidar (RPLidar S2L, through the `Oradar_S2L`
driver) and the button are unchanged.

The Obstacle Challenge (`src/cpp/object_detection_hiwonder_h10`) builds this
same layer from here (`hiwonder/`, `spike_hiwonder.cpp`, `parada_emergencia.cpp`,
`Oradar_S2L.cpp`) and reads the same calibration (`hiwonder_config.h`): whatever
is tuned here applies to both challenges.

## What changes

| File | Change |
|---|---|
| `hiwonder_config.h` | **All of the robot's calibration** |
| `spike.h` | The team's SPIKE API, now on top of `spike_compat` |
| `spike_hiwonder.cpp` | Board startup and hub translations |
| `parada_emergencia.cpp` | Ctrl+C: motors in COAST, servo torque off and process exit |
| `hiwonder/` | Copy of the `hiwonder_brick` library (as of 2026-10-04) |
| `main.cpp` | Startup only: `Spike_Hiwonder_Init()` instead of the relay and the REPL |
| `spike.cpp` | Still in the tree, but no longer compiled |

Adapter translations, covered by `make test`:

- `Spike_Reset_Gyro(int)` takes **decidegrees**, like `reset_yaw()` on the hub
  (`Oradar_S2L_Wall_Slope()` returns the angle x10). The layer works in degrees.
- `Spike_Turn_For_Degrees(dir, speed, grados)` turns with the steering at full
  lock, like `turn()` on the hub.
- `speed` 0..100 is mapped to rev/s in closed loop (100 = 5.7 rev/s), and
  `Spike_Advance_For_Degrees` measures distance with the M4 encoder.

## Build and test

```bash
make        # my_lidar_app
make test   # offline tests with a fake board: does not open the port
```

## Before the first run

With the wheels lifted. The calibration in `hiwonder_config.h` comes from the
test bench, and these checks determine whether the challenge turns towards the
correct side. The tools are in
`~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/` and
use the same port, so `my_lidar_app` must not be running.

1. **Yaw sign.** With `python3 imu_live.py`, turn the robot by hand to the
   right (clockwise seen from above): the yaw must **decrease**, as on the
   hub. The challenge depends on this: after turning right it continues with
   reference -90.
2. **Steering side.** With `python3 servo_hx12h.py --goto 605 --si-quiero-mover`
   (the right position in `hiwonder_config.h`), the wheels must point to the
   **right**. Return to centre with `--goto 750` (180°).
3. **Distance.** `Spike_Advance_For_Degrees(100, 1350, 0)` must cover the same
   distance as with the hub. If it does not, adjust `kGradosSpikeATicks`.

If a check fails, use this table:

| 1. Yaw | 2. Steering | What to do |
|---|---|---|
| correct | correct | Nothing: the calibration is consistent |
| correct | reversed | Swap `kDireccionDerecha` and `kDireccionIzquierda` |
| reversed | either | The board's yaw is inverted relative to the hub. It is corrected in `hiwonder_brick`, not here, because the layer reads it directly |

## Run

```bash
WRO_MAX_RPS=1.5 ./my_lidar_app   # first tests, slow
./my_lidar_app                   # competition speed, 5.7 r/s
```

At startup it centres the steering and calibrates the gyroscope for 5 s: **the
robot must stay still** until the button LED blinks. `WRO_MAX_RPS` lowers the
speed limit without recompiling; distances and angles do not change.

**First thing to watch on the track:** the lidar loops call `Spike_Forward`
every 2 ms, and the IMU arrives at 20 Hz. The D term of the PD only sees a
change in 1 of every ~25 calls, and applies it all at once. If the steering
oscillates on the straights, this is the cause.

## Emergency stop

The firmware **has no command watchdog**: if the program dies with the motor
running, the motor keeps going at the last speed. That is why
`parada_emergencia.cpp` is installed in `Spike_Hiwonder_Init()`, right after
opening the board and before moving anything.

**Ctrl+C stops the robot** at any time, including in the middle of a turn or
an advance:

- all four motors are left in **COAST** (the firmware sets both H-bridge
  inputs to 0; this also cancels a position HOLD);
- the steering servo is left **with torque off**, so it does not push the
  linkage against a stop (`kSoltarDireccionAlParar` in `hiwonder_config.h`);
- the program exits with code 130 without returning to `main.cpp`, so no loop
  sends another speed after the stop.

The same happens with `kill`, when the terminal or the SSH session is closed,
with Ctrl+Z (which previously suspended the process with the motor running) and
if the program crashes (segfault, abort). Before `Spike_Hiwonder_Init()`, Ctrl+C
still interrupts the wait for the lidar as usual.

It does not cover `kill -9` or a power loss on the Raspberry Pi. For those cases:

```bash
python3 ~/Jetson_cerebro/robotica/hiwonder_brick/hiwonder-analisis-estatico/tools/motor_move.py --stop
```

or cut the 12 V supply.
