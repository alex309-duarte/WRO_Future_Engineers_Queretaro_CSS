# On-device source code (`src/ondevice`)

C and C++ sources of the **Open Challenge** program (`main.cpp`) and of the hardware layer that both challenges share.

## Files

| File | What it does |
|---|---|
| `main.cpp` | Open Challenge mission (`Recorrido()`): three laps using the LiDAR and the gyro. |
| `Oradar_S2L.cpp` / `.h` | LiDAR driver used by design v7 for the Slamtec RPLidar S2L (the `Oradar_S2L_*` names are kept from design v6): 360-slot distance buffer and wall helpers. |
| `RPLidar_S2L.cpp` / `.h` | Original RPLidar S2L driver of the first prototype, kept for reference (not built). |
| `spike_compat.cpp`, `spike_hiwonder.cpp`, `spike.h` | SPIKE-compatibility layer: keeps the `Spike_*` motion API of the missions and translates it to the Hiwonder board. |
| `hiwonder/` | Board protocol library (`hiwonder_brick`): motor, bus servo and IMU of the Hiwonder ROS Robot Controller. |
| `hiwonder_config.h` | Calibration: steering centre and limits, speed, gyro calibration time. |
| `parada_emergencia.cpp` / `.h` | Emergency stop: motors to coast, servo torque off and LiDAR stopped on Ctrl+C, kill or crash. |
| `rasp_gpio.cpp` / `.h` | Start button and LED on the Raspberry Pi GPIO. |
| `lidar_display.cpp` / `.h` | Optional debug window with the LiDAR points. |
| `tests/` | Offline tests against a fake board (`make test`, 53 checks). |
| `spike.cpp` | SPIKE Prime hub driver of design v6, kept for reference (not built for design v7). |
| `rplidar_sdk/` | Slamtec RPLIDAR SDK (Git submodule). |
| `oradar_sdk/` | Oradar SDK, used only by the design v6 build. |

## More information

- Flowchart, state machine and thresholds of the Open Challenge: [main README, section 6.2](../../README.md#software).
- Build and run: [main README, section 9.5](../../README.md#setup-instructions).
- Board layer, checks before the first run and troubleshooting: [`HIWONDER.md`](HIWONDER.md).
- Wiring diagram: [`schemes/hardware/wiringSchematics_v7.png`](../../schemes/hardware/wiringSchematics_v7.png).

## Technical summary

- **Languages:** C and C++
- **Platform:** Raspberry Pi 5 with the Hiwonder ROS Robot Controller (firmware `pos-v8`, see [`firmware/hiwonder_pos-v8/`](../../firmware/hiwonder_pos-v8))
- **Dependencies:** Slamtec RPLIDAR SDK, OpenCV, libgpiod
