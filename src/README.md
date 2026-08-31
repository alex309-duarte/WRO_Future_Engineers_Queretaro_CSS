# object_detection_original_h10

Camera + lidar navigation app for the WRO Future Engineers robot, running on a
Raspberry Pi 5 with a Hailo-10H AI accelerator. It runs a Hailo inference
pipeline (traffic-light / cube detection from the camera) alongside an
Oradar MS200 lidar-based wall-following and cube-avoidance state machine that
drives the robot through a LEGO SPIKE Prime hub.

## 1. Module table

Files under `src/cpp/object_detection_original_h10/`:

| File | Module | Description |
|---|---|---|
| `object_detection.cpp` | Main application | Parses CLI args, loads the Hailo model (`HailoInfer`), wires the preprocess -> inference -> postprocess pipeline threads, and spawns `Obstacle_Challenge_Thread`. `postprocess_callback` draws detections and also runs the lidar wall-segmentation/orientation logic (`Select_Wall`, `Fit_Line_Orientation`, `Slope`, `Distance_To_Wall`) used for wall following. The bulk of the file implements the Obstacle Challenge state machine: cube-color decision logic (`Desicion`, `Corner_Case`), cube avoidance maneuvers (`esquivar_cubos_1/2/middle`, `avoid_cube_start_section`), and the angle/hypotenuse geometry used to aim at each cube (`calculte_angle_section_start_clockwise[_chr]`, `calculte_angle_section_start_counterclockwise`). |
| `Oradar_S2L.h` / `Oradar_S2L.cpp` | Oradar MS200 lidar driver | Owns the connection to the Oradar lidar (`Oradar_S2L_Init_Lidar`, background `Oradar_S2L_Lidar_Writer_Thread` that fills a 360-slot distance buffer), exposes the buffer (`Oradar_S2L_Get_Buffer`), degree/radian conversions, and a set of wall-relative navigation primitives (`Advance_And_Detect_Side`, `Advance_Until_Left/Right_Gap`, `Advance_And_Measure_Left/Right_Slope`, `Correction_For_Triangles_Left/Right`, `Slope`/`Slope2` least-squares wall-angle fits, `Reconcile_Readings`, `Average`). `RP_TO_ORADAR_IDX` translates the logical `LidarSide` angle (`FRONT`/`RIGHT`/`BACK`/`LEFT`, defined in `common_var.h`) into the Oradar's raw buffer index. |
| `spike.h` / `spike.cpp` | SPIKE Prime driver | Serial link (over USB, `/dev/ttyACM0`) to the LEGO SPIKE Prime hub. Sends Python source lines as serial commands (`Spike_Send_Serial_Data`) to define on-hub motor-control routines at boot (`Spike_Initialize_Libraries`), then triggers them for turns, straight advances, centering, gyro reset/read, etc. |
| `rasp_gpio.h` / `rasp_gpio.cpp` | Raspberry Pi GPIO | Uses `libgpiod` to read the start button, drive the status LED, and pulse the relay that powers on the SPIKE hub (`Rasp_Gpio_Init`, `Rasp_Gpio_Wait_For_Button`, `Rasp_Gpio_Power_On_Spike`, `Rasp_Gpio_Clean`). |
| `common_var.h` | Shared constants | `direction`, `Color_traffic_light`, `Cube_number[_chr]`, `Brake_type`, and `LidarSide` enums plus the global `terminating_main` flag shared across the lidar/Spike/GPIO/main modules. |
| `utils/utils.hpp` / `utils/utils.cpp` | Hailo postprocessing helpers | Bounding-box drawing, NMS output parsing (`parse_nms_data`) into `NamedBbox`, and COCO class name/color lookup helpers used by `postprocess_callback`. |

**External dependencies used but not in this folder** (wired in via `CMakeLists.txt`, code lives in `src/cpp/common/`): `toolbox.hpp/.cpp` and `hailo_infer.hpp/.cpp` (shared Hailo pipeline plumbing: arg parsing, preprocess/inference/postprocess thread runners) and `resources_manager.hpp/.cpp` (resolves a `--net` model name/path to a local `.hef`, downloading it if needed per `config/resources_config.yaml`). The Oradar SDK (`oradar_sdk/`) and the `yaml-cpp`/`curl` submodules (`../external/`) are vendored source trees pulled in by `CMakeLists.txt` via `add_subdirectory`.

## 2. Development history (this session)

Changes made to this project while porting the lidar navigation code developed
and tuned in the sibling `src/ondevice/` project:

- Renamed `Oradar_S2L_Grados_A_Radianes` -> `Oradar_S2L_Degrees_To_Radians`, `Oradar_S2L_Radianes_A_Grados` -> `Oradar_S2L_Radians_To_Degrees`, and `Oradar_S2L_Avanzar_Hasta_La_Distancia` -> `Oradar_S2L_Advance_Until_Distance` (kept its existing `Brake_type` parameter and `terminating_main`/raw-index-270 behavior; this one is a project-specific variant, not swapped for `ondevice`'s simpler version). Updated all ~16 call sites in `object_detection.cpp` to match.
- Added the `LidarSide{FRONT=0,RIGHT=90,BACK=180,LEFT=270}` enum to `common_var.h`, and `ORADAR_ANGLE_OFFSET`/`RP_TO_ORADAR_IDX` to `Oradar_S2L.h` -- reusing the `270`-degree offset already confirmed empirically for this robot by `Select_Wall` in `object_detection.cpp`.
- Ported 11 new navigation functions from `ondevice/Oradar_S2L.cpp` into this project: `Advance_And_Detect_Side`, `Advance_Until_Left_Gap`, `Advance_Until_Right_Gap`, `Advance_And_Measure_Left_Slope`, `Advance_And_Measure_Right_Slope`, `Correction_For_Triangles_Left`, `Correction_For_Triangles_Right`, `Reconcile_Readings`, `Slope`, `Slope2`, `Average` -- rewritten to index `oradar_shared_buffer` through `RP_TO_ORADAR_IDX` instead of the raw literals used in `ondevice`.
- Deliberately left `RPLidar_S2L.*` out of this project (Oradar-only here; no RPLidar SDK is vendored in this tree).
- Resolved a repo-wide merge conflict from a teammate's WRO-template folder reorganization (`RM`-prefixed renames) that collided with this file; kept this project's content, since the remote copy predated the Oradar work above (it was missing `esquivar_cubos` entirely and still used the old Spanish function names).

Related change in the **sibling `src/ondevice/` project** (its own `Makefile`, not this project's `CMakeLists.txt`, which was not modified this session): added a `LIDAR` build switch (`make` defaults to the RPLidar S2L driver, `make LIDAR=oradar` builds against `Oradar_S2L.cpp`/the vendored `oradar_sdk` instead), selected at compile time via `-DUSE_ORADAR` and a `LIDAR_FN(name)` macro in `main.cpp` that expands to `Oradar_S2L_name` or `RPLidar_S2L_name`. `object_detection_original_h10` has no such switch -- it links `Oradar_S2L.cpp` unconditionally (see `CMakeLists.txt` `SOURCES`).

## 3. Running this code on another Raspberry Pi 5 + Hailo-10H

1. **Flash the OS.** Raspberry Pi OS 64-bit (Bookworm or newer) on the Pi 5, via Raspberry Pi Imager. Enable SSH/serial as needed.
2. **Install the Hailo-10H stack.** Follow Hailo's Raspberry Pi setup for the AI HAT/M.2 Hailo-10H accelerator: install the `hailo-all` (or Hailo-10H specific) apt packages / PCIe driver + firmware + `HailoRT` runtime + `hailortcli` from Hailo's Raspberry Pi apt repo, then reboot and confirm the device is detected:
   ```bash
   hailortcli fw-control identify
   ```
   The HailoRT version installed here must match (or be compatible with) the Hailo Dataflow Compiler version used to produce the `.hef` files (see part 4).
3. **Install build dependencies.**
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake git libopencv-dev libgpiod-dev pkg-config
   ```
4. **Clone the repo with submodules** (needed for `yaml-cpp`/`curl` under `src/cpp/external/`):
   ```bash
   git clone --recurse-submodules https://github.com/alex309-duarte/WRO_Future_Engineers_Queretaro_CSS.git
   # or, if already cloned:
   git submodule update --init --recursive
   ```
5. **Place the `.hef` model(s).** Put the compiled model (see part 4) under `src/HailoModels/`, or let `resources_manager` resolve/download it per `src/config/resources_config.yaml` (`hailo10h` architecture entries).
6. **Check the hardcoded visualization-config fallback path.** `main()` in `object_detection.cpp` falls back to a hardcoded path (`/home/maker/WRO_Hailo10H_Compatible/software/cpp/object_detection_original_h10/visualization_config.yaml`) if the `WRO_VISUALIZATION_CONFIG` env var isn't set. On a fresh Pi that path won't exist -- either export the env var, or update the fallback:
   ```bash
   export WRO_VISUALIZATION_CONFIG=/absolute/path/to/src/cpp/object_detection_original_h10/visualization_config.yaml
   ```
7. **Build.**
   ```bash
   cd src/cpp/object_detection_original_h10
   mkdir -p build/h10_original && cd build/h10_original
   cmake -S ../.. -B .
   cmake --build . -j"$(nproc)"
   ```
8. **Wire up the hardware:**
   - SPIKE Prime hub: USB cable, appears as `/dev/ttyACM0` (see `spike.cpp`).
   - Oradar MS200 lidar: USB-serial, `/dev/ttyUSB0` at 230400 baud (see `Oradar_S2L.cpp`).
   - GPIO (BCM numbering, see `rasp_gpio.h`): button on GPIO4, status LED on GPIO3, SPIKE power relay on GPIO2.
9. **Run.**
   ```bash
   ./object_detection --net <model_name_or_path.hef> [other args parsed by toolbox.cpp]
   ```

## 4. From dataset to a working `.hef` (beginner walkthrough)

This section is written for someone doing this for the first time. A `.hef` file
is just "a neural network, packaged in the one format the Hailo chip understands."
You can't hand the Hailo chip a normal file straight out of training (an `.onnx`
or `.pt` file) -- you first have to *compile* it, similarly to how you compile
C++ source into a binary the CPU can run. That compiling step is what most of
this section is about.

There are two separate computers involved, and it's easy to mix them up:

- **Your laptop/desktop (x86_64, Windows/Linux/Mac)** -- where you train the
  model and compile it into a `.hef`. This needs a lot of RAM and, for
  reasonable training speed, a GPU (Google Colab gives you one for free).
- **The Raspberry Pi 5** -- where the finished `.hef` actually *runs*, inside
  this repo's `object_detection` program. The Pi never trains or compiles
  anything; it only runs the already-compiled file.

### 4.1 Get the labeled dataset

The images and annotations used to train the traffic-light/cube detector live
in a separate repo:

```bash
git clone https://github.com/alex309-duarte/WRO_FutureEngineers_Q.git
```

Inside it, `model_training/DataSet_labelStudio/` has:

- `images/` -- the photos.
- `labels/` -- one `.txt` file per image, in YOLO format (one line per object:
  `class_id x_center y_center width height`, all normalized 0-1).
- `classes.txt` -- the list of class names, in the same order as the
  `class_id` numbers used in `labels/`.

This is a *raw* Label Studio export: everything is in one folder, not yet
split into a training set and a validation set, and there's no `data.yaml`
(the small config file that tells YOLO where the images/labels live and what
the classes are called). You'll create both of those in the next step.

### 4.2 Train the YOLOv8n model

This project's own README documents training via Google Colab, using
Luxonis's ready-made notebook:
[YoloV8_training.ipynb](https://colab.research.google.com/github/luxonis/depthai-ml-training/blob/master/colab-notebooks/YoloV8_training.ipynb).
Colab gives you a free cloud GPU in the browser -- no local install needed for
this step.

1. Open the notebook link above and make a copy to your own Google Drive
   (`File -> Save a copy in Drive`) so you can edit and re-run it.
2. Upload the `DataSet_labelStudio` folder (or `images/` + `labels/` +
   `classes.txt`) to your Google Drive, and mount Drive in the notebook so it
   can read those files.
3. Follow the notebook's own steps to split the images into `train/` and
   `val/` subsets and generate a `data.yaml` pointing at them, using the
   class names from `classes.txt`.
4. Train. The settings this project's own trained model used (visible in
   `model_training/Training_model_and_results/args.yaml` in the dataset
   repo) were:
   - model: `yolov8n.pt` (the smallest/fastest YOLOv8 variant -- what you
     want on a small on-device accelerator like the Hailo-10H)
   - `epochs=100`, `batch=16`, `imgsz=320`
   - In the notebook (or a plain terminal with `pip install ultralytics`),
     this is the equivalent of:
     ```bash
     yolo detect train model=yolov8n.pt data=data.yaml epochs=100 batch=16 imgsz=320
     ```
   Training can take anywhere from tens of minutes to a few hours depending
   on dataset size and Colab's GPU allocation for that session -- this is
   expected, just let it run.
5. When it finishes, your trained weights are at
   `runs/detect/train/weights/best.pt`. Download that file -- it's what the
   next step converts.

### 4.3 Turn `best.pt` into a `.hef`

There are two ways to do this. **Use option A unless you have a specific
reason not to** -- it's one command instead of a whole toolchain install.

#### Option A (recommended): one-command export via Ultralytics

Recent versions of the `ultralytics` Python package can export straight to a
Hailo `.hef`, handling the ONNX conversion and the Hailo compiler internally.
On your laptop/desktop (not the Pi):

```bash
pip install ultralytics
pip install /path/to/hailo_dataflow_compiler-<version>-<pyver>-linux_x86_64.whl
```

(You still need the Hailo Dataflow Compiler `.whl` -- see the version note
below for which one to download from the
[Hailo Developer Zone](https://hailo.ai/developer-zone/) (free account
required).)

```python
from ultralytics import YOLO

model = YOLO("best.pt")
model.export(format="hailo", name="hailo10h", imgsz=320, data="data.yaml")
```

This produces a `.hef` file targeting the Hailo-10H directly.

#### Option B (manual/classic): `hailomz` step-by-step

This is the flow from Cytron's
[Raspberry Pi AI Kit: ONNX to HEF Conversion](https://www.cytron.io/tutorial/raspberry-pi-ai-kit-onnx-to-hef-conversion)
guide (written for the older Hailo-8L kit) -- useful if you want more control
over each step, or option A isn't available for your `ultralytics` version.
**Do this on a separate x86_64 Linux machine, not the Pi**: the Dataflow
Compiler needs real CPU/RAM (Hailo recommends >=32 GB RAM for the
quantization/calibration step) and doesn't run on the Pi's ARM CPU.

1. **Install host prerequisites:**
   ```bash
   sudo apt update
   sudo apt install -y python3-pip python3.10-venv build-essential graphviz graphviz-dev
   pip install pygraphviz
   ```
2. **Create and activate a virtual environment** (an isolated Python install
   so this doesn't clash with other Python projects on your machine):
   ```bash
   python3.10 -m venv hailo_dfc_env
   source hailo_dfc_env/bin/activate
   ```
3. **Install the Hailo Dataflow Compiler** (see the version note below for
   which one), then sanity-check it installed correctly:
   ```bash
   pip install hailo_dataflow_compiler-<version>-<pyver>-linux_x86_64.whl
   hailo -h
   ```
4. **Get the Hailo Model Zoo** (a collection of ready model configs/scripts
   that `hailomz` uses):
   ```bash
   git clone https://github.com/hailo-ai/hailo_model_zoo.git
   cd hailo_model_zoo
   pip install -e .
   ```
5. **Export `best.pt` to ONNX first** (`hailomz` wants ONNX in, not `.pt`;
   `opset=11` specifically, or the Hailo compiler will fail to read the file):
   ```bash
   yolo export model=best.pt format=onnx imgsz=320 opset=11
   ```
6. **Compile the ONNX into a `.hef`, targeting Hailo-10H** (`--calib-path`
   should point at a folder of representative images -- e.g. a subset of your
   own `val/` images -- the compiler uses these to calibrate INT8 quantization
   accuracy; `--classes` is how many object classes your model detects):
   ```bash
   hailomz compile <model_name> \
     --ckpt=<path/to/best>.onnx \
     --hw-arch hailo10h \
     --calib-path <path/to/calibration/images> \
     --classes <N> \
     --performance
   ```
   This produces `<model_name>.hef` in the working directory.

### 4.4 Dataflow Compiler / HailoRT version compatibility -- read this before installing

The Hailo-8/8L and the Hailo-10H are different chip generations and **use
different major versions of the toolchain** -- installing the wrong one is
the single most common reason this whole process fails partway through:

| Target chip | Dataflow Compiler (DFC) | HailoRT (runtime, installed on the Pi) |
|---|---|---|
| Hailo-8 / Hailo-8L | v3.x (e.g. 3.28-3.33) | v4.x (e.g. 4.17-4.23) |
| **Hailo-10H / Hailo-15 (this project)** | **v5.x** | **v5.x** |

For this project's hardware, install **DFC v5.x on your training machine**
(the [Hailo Developer Zone](https://hailo.ai/developer-zone/) download page
lets you pick the version) and make sure the **HailoRT you install on the
Raspberry Pi in part 3, step 2 is also v5.x, and as close as possible to the
same version** -- e.g. DFC 5.3.0 pairs with HailoRT 5.3.0. Mismatched major
versions between the compiler and the on-device runtime are a common source
of `.hef` files that fail to load on the Pi even though compilation "succeeded."
If in doubt, check the exact pairing on the Version Compatibility Table page
in the Hailo AI Software Suite documentation.

### 4.5 Deploy

Copy the resulting `.hef` onto the Pi 5, into `src/HailoModels/` (or wherever
`--net`/`resources_config.yaml` expects it), and pass its name/path to
`--net` when running `object_detection` (part 3, step 9).
