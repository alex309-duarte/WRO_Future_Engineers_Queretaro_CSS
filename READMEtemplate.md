# CSS - WRO Future Engineers Querétaro



This repository contains the engineering documentation for **CSS**'s autonomous vehicle for the **WRO Future Engineers**, 2026 season.

## Run this command to run obstacle challenge with no display
sudo /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_original_h10/build/h10_original/object_detection   --net /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/HailoModels/roboflow_yolov8n_wro_h10.hef   --input rpi --no-display

## Run this command to run obstacle challenge with display
sudo /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/cpp/object_detection_original_h10/build/h10_original/object_detection   --net /home/maker/WRO_Future_Engineers_Queretaro_CSS/src/HailoModels/roboflow_yolov8n_wro_h10.hef   --input rpi

## Table of Contents

1. [Our Team](#our-team)
2. [Challenge Overview](#challenge-overview)
3. [Our Robot](#our-robot)
    - [Sensors](#sensors)
    - [Mobility and Chassis](#mobility-and-chassis)
    - [Electronics](#electronics)
7. [Software](#software)
8. [Obstacle Management](#obstacle-management)
9. [Construction Guide](#construction-guide)
10. [Cost Report](#cost-report)
11. [Driving Video](#driving-video)
12. [Resources](#resources)
13. [License](#license)

---

## 1. Our Team <a name="our-team"></a>

<div align="center">
<img src="t-photos/t-photo.jpg" width="500">
<!-- Official team photo -->
</div>

### [Your name]
**Age:** [ ]
**Role:** [e.g. Captain, Software, Electronics, Mechanics]

[1-2 lines about your background/experience in robotics or the project]

> "[Personal quote or philosophy about the project, optional]"

---

### [Teammate's name]
**Age:** [ ]
**Role:** [ ]

[Brief description]

> "[Optional quote]"

*(Repeat this block for each team member)*

---

## 2. Challenge Overview <a name="challenge-overview"></a>

The **WRO Future Engineers** challenge requires teams to build a fully autonomous vehicle capable of:

- Navigating a track with randomized wall layouts
- Detecting and avoiding colored obstacles (green/red blocks), passing on the correct side based on color
- Performing a parallel parking maneuver
- Completing the Open Challenge (obstacle-free laps) and the Obstacle Challenge (with obstacles)

More info: [WRO Official Site](https://wro-association.org/) | [Future Engineers Rules](https://wro-association.org/wp-content/uploads/WRO-2025-Future-Engineers-Self-Driving-Cars-General-Rules.pdf)

*(Check that the rules link points to the current 2026 version)*

---

## 3. Our Robot <a name="our-robo"></a>

**Robot name:** [ ]

<table style="width:100%">
<tr>
<td align="center"><img src="v-photos/front.jpg" width="200"><br>Front</td>
<td align="center"><img src="v-photos/back.jpg" width="200"><br>Back</td>
<td align="center"><img src="v-photos/left.jpg" width="200"><br>Left</td>
</tr>
<tr>
<td align="center"><img src="v-photos/right.jpg" width="200"><br>Right</td>
<td align="center"><img src="v-photos/top.jpg" width="200"><br>Top</td>
<td align="center"><img src="v-photos/bottom.jpg" width="200"><br>Bottom</td>
</tr>
</table>

[Brief general description of the vehicle: dimensions, design philosophy, what sets it apart]

---

## 4. Sensors <a name="sensors"></a>

*Describe each sensor: what it measures, why you chose it, and how it's integrated into your system.*

### 4.1. RPLiDAR S2L
- **Function:** [360° distance measurement, wall/obstacle detection]
- **Key specs:** [range, scan frequency, angular resolution]
- **Why it was chosen:** [ ]

### 4.2. IMU
- **Model:** [e.g. BNO055, MPU6050, etc.]
- **Function:** [orientation/heading estimation, drift detection]
- **Integration:** [how it's fused with other readings — complementary filter, Kalman, etc.]

### 4.3. [Other sensor, if applicable: camera, encoders, color sensor, etc.]
- **Function:** [ ]
- **Specs:** [ ]

---

## 5. Mobility and Chassis <a name="mobility-and-chassis"></a>

### 5.1. Steering system
[Ackermann, servo, geometry, max turning angle]

### 5.2. Drivetrain
[Motor(s) used, gear reduction, wheels]

### 5.3. Chassis
[Material — 3D printed, structural PCB, etc. — and design decisions]

### 5.4. Assembly
[Relevant notes on mounting, weight distribution, center of gravity]

---

## 6. Electronics <a name="electronics"></a>

### 6.1. System architecture
[Diagram or description of how components communicate: Raspberry Pi 5 ↔ microcontroller ↔ sensors/actuators]

### 6.2. Microcontroller comparison
[If you evaluated several options — Teensy, Arduino, ESP32, etc. — and why you chose yours]

### 6.3. Power architecture
[Batteries, voltage regulators, circuit protection]

### 6.4. Motor driver
[Model, specs]

### 6.5. PCB design (if applicable)
[Tool used, layers, layout notes]

### 6.6. Schematic diagram
[Link or image to `schemes/hardware/`]

### 6.7. Wireless communication
[Protocol used — WiFi, Bluetooth — and its purpose: telemetry, remote debugging, configuration]

---

## 7. Software <a name="software"></a>

### 7.1. System architecture
[Data flow diagram: sensors → processing → decision → actuators. Mention whether you use ROS2, threads, or a custom architecture in C++]

### 7.2. Languages and environment
- **Main language:** C++
- **Platform:** Raspberry Pi 5
- **Libraries/SDKs used:** [e.g. RPLiDAR SDK, IMU library, etc.]

### 7.3. Implemented algorithms
*Describe each key algorithm in enough technical detail:*

#### LiDAR processing
[How you filter/interpret scans — wall detection, corner detection, minimum distances]

#### Sensor fusion / heading estimation
[How you combine IMU + LiDAR (or others) to determine orientation/position]

#### Trajectory control (closed-loop)
[Type of controller — PID or other — input/output variables, gains]

#### Obstacle detection
[Method used — vision, color, LiDAR — and decision logic]

### 7.4. Communication protocol
[If there's a secondary microcontroller, describe the serial frame format, baud rate, checksum, etc.]

### 7.5. State machine
[Robot states: startup, normal navigation, obstacle avoidance, parking, stopped]

### 7.6. File structure
```
src/
├── [folder]/     # [what it contains]
├── [folder]/     # [what it contains]
```

### 7.7. Execution commands
```bash
# Example — replace with your actual commands
sudo ./build/your_program --config config.yaml
```

---

## 8. Obstacle Management <a name="obstacle-management"></a>

### 8.1. Detection methods
[Sensors/algorithms used to identify green/red blocks]

### 8.2. Avoidance logic
[Passing rule: green → left, red → right, for example]

### 8.3. Parking maneuver
[How it detects the space and executes the parallel parking]

### 8.4. Safety systems
[Emergency stop, sensor failure handling, automatic recovery]

---

## 9. Construction Guide <a name="construction-guide"></a>

### 9.1. General steps
1. 3D design
2. Part fabrication/printing
3. Mechanical assembly
4. Wiring and electronics
5. Software installation
6. Calibration
7. Testing

### 9.2. Tools used
- [3D printer, model]
- [Soldering tools]
- [Other relevant tools]

---

## 10. Cost Report <a name="cost-report"></a>

| Component | Qty | Unit Cost (MXN) | Total (MXN) |
|---|---|---|---|
| Raspberry Pi 5 | 1 | $ | $ |
| RPLiDAR S2L | 1 | $ | $ |
| IMU [model] | 1 | $ | $ |
| [Microcontroller] | 1 | $ | $ |
| [Motor] | 1 | $ | $ |
| [Steering servo] | 1 | $ | $ |
| [Battery] | 1 | $ | $ |
| [Chassis/3D printing] | - | $ | $ |
| **Total** | | | **$ ** |

---

## 11. Driving Video <a name="driving-video"></a>

[Link to demonstration video — upload it to `video/video.md` or link it directly here]

---

## 12. Resources <a name="resources"></a>

- [WRO Official Site](https://wro-association.org/)
- [Future Engineers Rules](https://wro-association.org/)
- [Team Repository]([your link here])

---

## 13. License <a name="license"></a>

```
MIT License
Permission is hereby granted, free of charge, to any person obtaining a copy of this software.
```

---

> *Document maintained by [Team Name] | Last updated: [month 2026]*
