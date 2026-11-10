# MOTOR_EXT protocol (relative position control of one motor)

## Version 3 (2026-10-04, `pos-v8`, on the board): `HOLD`

`0x05 HOLD`. 10-byte request:
`cmd, motor_id, max_rps_f32, tolerance_u16, max_pulse_u16`.
Holds the current counter with no timeout, like SPIKE's `motor.HOLD`. If the
shaft is displaced by more than `tolerance`, it returns using the same P law as
`MOVE_RELATIVE`, limited to `max_rps`. The PWM is limited to `max_pulse`
(300..1000). If it pushes at the cap without moving for 3 s, it releases with
PWM at zero and state **7 `stalled`**; while holding, the state is
**6 `holding`**, with `active = 1`. It is cancelled by `CANCEL`,
`MOVE_RELATIVE`, a `0x03` velocity command to that motor, or a stop.
Response: the 20-byte status report with `cmd = 0x05`.
`CAPABILITIES` reports version 3. With v2 firmware the command returns state 5
(`invalid`).

## Version 2 (2026-10-04): the position motor is M4

The `pos-v5` image moves position control from M1 to **M4**
(`POSITION_MOTOR_INDEX 3` in `global_conf.h`). The frame format does not
change; three values change:

- `CAPABILITIES` reports **version 2** and `motor_mask = 1 << POSITION_MOTOR_INDEX`
  (`0x08` for M4). Ticks/revolution are read from the motor object (1320).
- `motor_id` in `MOVE_RELATIVE`, `CANCEL`, `ZERO` and `STATUS` must be
  `POSITION_MOTOR_INDEX` (**3**). Any other value returns state `invalid` (5),
  with `motor_id = 3` in the response.
- A `0x03` velocity command (sub 0 or 1) addressed to M4 cancels the position
  move in progress (previously, to M1).

In addition, `0x03` sub 0 and sub 1 no longer index `motors[]` without bounds
checking: they ignore `motor_id > 3` and frames whose length does not cover
`motor_num` elements.

The rest of this document describes version 1 (M1, `pos-v4`); wherever it says
M1 or `motor_id 0`, read M4 and `motor_id 3` for v2.

## Version 1 status

The `extended_m1` image keeps the factory protocol and adds function `0x0B`.
It was flashed and validated on the bench on 2026-08-22 (`pos-v4`).

The USB protocol keeps the existing format:

`AA 55 | function | length | data | CRC8-Maxim`

The current extension is a deliberately small MVP. It only controls M1 and does
not yet include a trapezoidal profile, stall detection or its own watchdog.

## Function `0x0B` (`MOTOR_EXT`)

All multi-byte integers and `float32` values are encoded little-endian.

### `0x00 CAPABILITIES`

Request: one byte `cmd=0x00`.

8-byte response:

`cmd, version, motor_mask, reserved, ticks_per_rev_u32`

For this image: version 1, mask `0x01` and 1320 ticks/revolution.

### `0x01 MOVE_RELATIVE`

14-byte request:

`cmd, motor_id, delta_ticks_i32, max_rps_f32, tolerance_u16, timeout_ms_u16`

- `motor_id`: only `0`, corresponding to the physical M1 connector.
- `delta_ticks`: relative displacement from the current counter.
- `max_rps`: maximum magnitude of the velocity-loop setpoint.
- `tolerance`: allowed error in ticks.
- `timeout_ms`: 1 to 60000 ms.

For the JGB37-520R30-12 1:30, 1320 ticks per output-shaft revolution are used:

`ticks = round(degrees * 1320 / 360)`

### `0x02 CANCEL`

Request: `cmd, motor_id`. Stops M1 and leaves the state `cancelled`.

### `0x03 ZERO`

Request: `cmd, motor_id`. Stores the current counter as the relative zero
without resetting the encoder's hardware timer.

### `0x04 STATUS`

Request: `cmd, motor_id`.

The move, cancel, zero and status responses are 20 bytes long:

`cmd, motor_id, state, active, position_i32, target_i32, error_i32, measured_rps_f32`

States:

- `0`: idle
- `1`: moving
- `2`: reached
- `3`: cancelled
- `4`: timeout
- `5`: invalid

## Implemented control

TIM7 runs at 100 Hz:

1. an outer P loop converts the position error into a limited r/s setpoint;
2. the original velocity PID converts that setpoint into PWM;
3. `reached` is declared after five samples within tolerance and at low
   speed;
4. when the timeout expires, M1 is stopped and `timeout` is declared.

An original velocity command for M1 cancels any active position move,
keeping compatibility with existing software.

## First-test limits

The first powered test will be done with the wheel lifted, a current-limited
supply, 30 degrees (`110 ticks`), maximum `0.25 r/s`, a tolerance of 15 ticks
and a physical stop within reach. It will not yet be used to hold a load or to
steer the robot.

## Later evolution

A later version may add sequences, an `int64` counter, an acceleration
profile, periodic telemetry, a watchdog and stall detection. These features
must not be assumed to be available in the current image.
