# WRO Future Engineers - Startup Services

The robot can start either program at boot with a systemd service. Both
programs use the same hardware (camera, LiDAR, Hiwonder board, GPIO), so only
**one** of them may run at a time. Give both units `Conflicts=` on each other,
so starting one automatically stops the other.

| Service | Program (design v7) | `ExecStart=` |
|---|---|---|
| `object_detection.service` | Obstacle Challenge: camera + Hailo-8 + RPLidar S2L + Hiwonder board | `/home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_hiwonder_h10/run_hiwonder_h8.sh --no-display` |
| `my_lidar_app.service` | Open Challenge: RPLidar S2L + Hiwonder board | `/home/maker/WRO_Future_Engineers_Queretaro_CSS/src/ondevice/my_lidar_app --no-display` |

Build both programs first (README §9.5 and §9.6). `run_hiwonder_h8.sh` selects
the `build/h8_s2l` build and the `wro_h8_yolov8n_v4.hef` model and runs the
program from its build directory. `--no-display` disables the camera and LiDAR
windows, which a service cannot open.

Example unit file (`/etc/systemd/system/object_detection.service`):

```ini
[Unit]
Description=WRO FE Obstacle Challenge (design v7)
After=multi-user.target
Conflicts=my_lidar_app.service

[Service]
Type=simple
ExecStart=/home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_hiwonder_h10/run_hiwonder_h8.sh --no-display
Restart=no

[Install]
WantedBy=multi-user.target
```

`my_lidar_app.service` is the same with its own `Description`, `ExecStart`
and `Conflicts=object_detection.service`. After creating or editing a unit,
run `sudo systemctl daemon-reload`.

At start-up both programs centre the steering and calibrate the gyroscope
for 5 s, so the robot must stay still after power-on; the run itself starts
with the button (GPIO4).

<details>
<summary>Design v6 (national) services</summary>

1. **`object_detection.service`** ran
   ```
   /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_original_h10/build/h10_original/object_detection \
     --net /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/HailoModels/roboflow_yolov8n_wro_h10.hef \
     --input rpi --no-display
   ```
2. **`my_lidar_app.service`** ran
   ```
   /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/ondevice/my_lidar_app
   ```

</details>

Service unit files live in `/etc/systemd/system/`.

## Choosing which one runs on boot

Only "enable" the one you want to auto-start. Example, to boot into
`object_detection.service`:

```sh
sudo systemctl disable my_lidar_app.service
sudo systemctl enable object_detection.service
```

Or to boot into `my_lidar_app.service` instead:

```sh
sudo systemctl disable object_detection.service
sudo systemctl enable my_lidar_app.service
```

You can also leave both disabled if you don't want anything to auto-start,
and just start one manually when needed (see below).

## Basic commands

Replace `SERVICE` with either `object_detection.service` or `my_lidar_app.service`.

| Action | Command |
|---|---|
| Start now (without enabling on boot) | `sudo systemctl start SERVICE` |
| Stop | `sudo systemctl stop SERVICE` |
| Restart | `sudo systemctl restart SERVICE` |
| Enable on boot (auto-starts every startup) | `sudo systemctl enable SERVICE` |
| Enable AND start immediately | `sudo systemctl enable --now SERVICE` |
| Disable on boot (won't auto-start anymore) | `sudo systemctl disable SERVICE` |
| Disable AND stop immediately | `sudo systemctl disable --now SERVICE` |
| Check current status (running/stopped, recent log lines) | `sudo systemctl status SERVICE` |
| Check whether it's enabled on boot | `systemctl is-enabled SERVICE` |
| Check whether it's currently running | `systemctl is-active SERVICE` |

## Viewing logs

```sh
# Follow logs live (Ctrl+C to stop watching, does NOT stop the service)
sudo journalctl -u SERVICE -f

# Show last 50 lines
sudo journalctl -u SERVICE -n 50

# Show logs since last boot
sudo journalctl -u SERVICE -b
```

## Interrupting / stopping mid-run

- If you ran the program directly in a terminal (not via systemd), press
  **Ctrl+C**.
- If it is running as a systemd service (in the background, no terminal),
  use:
  ```sh
  sudo systemctl stop SERVICE
  ```
  `systemctl stop` sends `SIGTERM`. In design v7 both programs install the
  emergency stop (`src/ondevice/parada_emergencia.cpp`), which handles
  `SIGINT` (Ctrl+C) and `SIGTERM` the same way: the motors go to COAST, the
  steering servo torque is released, the LiDAR is stopped and the process
  exits.

## Editing a service file

1. Edit the file with sudo, e.g.:
   ```sh
   sudo nano /etc/systemd/system/object_detection.service
   ```
2. Reload systemd so it picks up the change:
   ```sh
   sudo systemctl daemon-reload
   ```
3. Restart the service to apply changes:
   ```sh
   sudo systemctl restart object_detection.service
   ```
