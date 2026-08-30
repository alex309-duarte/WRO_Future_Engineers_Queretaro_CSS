# Decisions - PCB Phase 2

## Closed baseline decisions
- Control architecture: local MCU + optional external host.
- Baseline MCU: `STM32F103RCT6` or pin-compatible equivalent.
- Battery baseline: `3S Li-ion/LiPo`.
- Active rails: `IN12V`, `VM`, `+5V`, `+3V3`.
- Primary programming/debug: `SWD`.
- Normal host communication: `UART`.
- IMU: `ICM20948`.
- Motor scope: only `Motor 1` is active in this revision.
- Out of active scope: `CAN`, `OLED`, `servo`, `M2`, `M3`, `M4`.

## Phase 2 implementation scope
Active sheets in this revision:
- `power`
- `mcu`
- `debug`
- `usb_uart`
- `imu`
- `motors_4x` implemented as `M1 only`
- `user_io`
- `connectors`

Inactive by scope and not wired in root sheet:
- `can.kicad_sch`
- `oled.kicad_sch`
- `servo.kicad_sch`

## Deviations and rationale
- The sheet file name `motors_4x.kicad_sch` was preserved for continuity, but only `M1` is implemented in this revision.
- `user_io` is kept active only for minimum reset/boot accessibility.
- `usb_uart` remains interface-only in this phase. No USB-UART bridge IC is committed yet.
- Ground domains `GND`, `DGND`, `AGND`, and `VGND` are exposed as explicit design domains. They are not shorted in this phase.
- The PDF reference is treated as topological guidance; this project scope follows the current baseline message when there is a scope conflict.

## IMU interface decision status
Current status: not fully frozen at electrical implementation level.

Phase 2 boundary decision:
- Keep both `IMU_SDA_SDI` and `IMU_SCL_SCLK` names to preserve I2C/SPI optionality.
- Keep `IMU_nCS` and `IMU_AD0_SDO` visible.

Short tradeoff:
- `I2C`: fewer MCU pins, simpler routing, easier shared bus.
- `SPI`: better throughput, lower latency, better for deterministic burst reads.

Provisional recommendation for next phase:
- Prefer `SPI` if high-rate inertial acquisition is required.
- Prefer `I2C` if simplicity and pin economy are prioritized.

## Power decisions intentionally left open
- Final battery connector: `XT30` vs terminal block.
- Exact 5 V buck regulator component.
- Exact 3.3 V regulator/LDO component.
- Real motor peak current and protection sizing.
- Final USB-UART implementation choice.

## Notes for next phase
- Add real MCU symbol and freeze pinout.
- Add real power conversion topology.
- Add motor driver/protection based on current target.
- Freeze IMU bus as `I2C` or `SPI` before selecting passives and pull-ups.
