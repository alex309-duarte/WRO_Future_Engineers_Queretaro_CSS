# On-Device Source Code (`src/ondevice`)

This directory contains the C and C++ source files and header libraries (`.c`, `.cpp`, `.h`) required to run the robot's control logic, sensor data acquisition, and actuation for the First Challenge.


## Directory Overview

* **Source Files (`.c` / `.cpp`):** Implement low-level control algorithms, finite state machine logic, peripheral drivers, and hardware communications.
* **Header Files (`.h`):** Define pin mappings, data structures, function prototypes, and global configuration constants.

## Flowcharts and State Diagrams

For a detailed view of the execution flow and system logic:

1. **Main README:** Refer to the root [`README.md`](../../README.md) for a high-level overview of the software architecture and system design.
2. **Schematics and Diagrams:** Visit the [`/schematics`](../../schematics) directory to access the system flowcharts and state machine diagrams.


## Technical Specifications

* **Programming Languages:** C / C++
* **Target Platform:** On-board embedded platform / RaspberryPi5-Lego Spike Prime
* **Dependencies:** Local header files and platform-specific libraries configured within the source tree.