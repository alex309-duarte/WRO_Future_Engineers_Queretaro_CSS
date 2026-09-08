# Models

This directory contains the complete 3D assembly and the individual parts that make up the robot. It also includes the `.stl` files used for 3D printing essential structural components, such as the main chassis and various mounts.

## Design Objectives

The primary objective behind the robot's mechanical design was maximum space optimization. This approach was essential to achieve two critical goals:
* **Center of Gravity:** Lowering the center of gravity as much as possible to ensure maximum stability during high-speed maneuvers on the track.
* **Sensor Placement:** Lowering the LiDAR sensor enough to accurately detect the 10 cm high track walls across a full 360-degree field of view.

### The LiDAR Challenge
Reaching the final design required an extensive testing process, evaluating multiple component layouts. One of the most persistent challenges was the LiDAR positioning. Because the sensor has a built-in pitch angle of 2 degrees, placing it too high caused the laser to overshoot the 10 cm walls at longer distances. 

To resolve this, the internal layout was heavily optimized, successfully lowering the LiDAR to a height where it reliably detects all track boundaries without blind spots.

## Engineering and Kinematics

### Ackermann Steering
The vehicle incorporates Ackermann steering geometry, allowing the robot to navigate turns smoothly without wheel slip or drag. Through this mechanical design, a maximum steering angle of 45.44 degrees was achieved.

### Wheelbase-to-Track Ratio
To guarantee driving stability and reliable cornering, a wheelbase-to-track ratio of 1.34 was implemented. This proportion was intentionally selected to closely mimic the 1.4 ratio commonly used in real-world automotive design, ensuring optimal handling characteristics.

## Maintenance and Accessibility

For competition environments, quick maintenance is crucial. The mechanical design optimizes ergonomics and accessibility:
* **Battery Replacement:** The chassis allows for fast and effortless swapping of the LEGO Spike batteries.
* **Accessible Electronics:** All connectors, wiring, and modules are strategically placed within reach, enabling rapid troubleshooting and repairs between competition rounds.

## Software and Resources

* **CAD Software:** The 3D modeling and assembly were carried out using SolidWorks 2026, chosen due to the team's extensive experience with this software.
* **CAD Libraries:** Platforms like GrabCAD were utilized to source 3D models for standard electronic components, including the Raspberry Pi and Grove modules.
* **Manufacturer Documentation:** External components, such as the Oradar LiDAR, were integrated using official technical documentation and dimensions provided by the manufacturers.

## Contents

* **`Iterations/`**: Contains previous design iterations, early prototypes, and testing models used throughout the development process.
* **`Final_assembly/`**: Contains the final 3D CAD assembly of the robot, representing the definitive competition model.
* **`Stl/`**: Contains the `.stl` files of all the custom components that were physically 3D printed to build the robot.