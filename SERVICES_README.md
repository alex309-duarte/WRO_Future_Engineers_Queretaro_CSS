# WRO Future Engineers - Startup Services

Two systemd services are installed on this Raspberry Pi. They both use the
same hardware (camera / lidar / serial), so only **one** of them should be
running at a time. They are configured with `Conflicts=` so starting one
will automatically stop the other.

1. **`object_detection.service`**
   Runs:
   ```
   /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_original_h10/build/h10_original/object_detection \
     --net /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/HailoModels/roboflow_yolov8n_wro_h10.hef \
     --input rpi --no-display
   ```

2. **`my_lidar_app.service`**
   Runs:
   ```
   /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/ondevice/my_lidar_app
   ```

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

- If you ran the binary directly in a terminal (not via systemd), press
  **Ctrl+C**. `object_detection` has a `SIGINT` handler for graceful shutdown.
- If it's running as a systemd service (in the background, no terminal),
  Ctrl+C does nothing because there's no attached terminal. Use:
  ```sh
  sudo systemctl stop SERVICE
  ```
  Note: `systemctl stop` sends `SIGTERM` by default, not `SIGINT`. If you need
  the exact same graceful-shutdown behavior as Ctrl+C, add a line
  `KillSignal=SIGINT` under `[Service]` in the `.service` file and run
  `sudo systemctl daemon-reload`.

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
