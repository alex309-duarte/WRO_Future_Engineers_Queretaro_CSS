# Assembly instructions

**WRO Future Engineers 2026 · Team CSS · "The Maker"**

Build order for the robot. Each step has one photo and says what to do and what to check. In the photos of the whole robot, the **front** (steering and controller board) is at the bottom of the picture unless the step says otherwise.

## Before you start

- **Print the parts.** The STL files and the number of pieces are in [models/cad/Stl/](../../models/cad/Stl/README.md). We printed them on a Creality K1C with a 0.4 mm nozzle in Creality Hyper ABS, with the default slicer profile. The holes for the heat-set inserts are already in the STL files.
- **Get the bought parts.** Everything you buy, including the LEGO parts and the screw, standoff and insert kits, is in the [Bill of Materials](../../README.md#bom). The parts list below gives how many of each small part the robot needs.
- **Wiring:** keep the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png) and the [connection table](../../README.md#connections) open for every electrical connection.

### Tools and materials

These are needed to build the robot but are not parts of it, so they are not in the Bill of Materials.

| Tool or material | Used for |
|---|---|
| Spot welding machine and nickel strips | Joining the battery cells (step 1) |
| Soldering iron, solder wire and flux paste | Soldering wires to the nickel strips and to the BMS; pressing in the heat-set inserts |
| Multimeter | Checking the polarity and the voltage of the battery pack |
| Small Phillips screwdrivers | All the screws |
| Side cutters | Cutting nickel strips, wires and cable ties |
| Insulating tape | Insulating the battery pack |
| Instant glue (cyanoacrylate) | Keeping the rear axle from sliding out (step 17) |

> [!WARNING]
> **Battery safety.** Lithium-ion cells can deliver very high currents. A short circuit can burn wires, damage the cells or start a fire.
>
> - Never touch both ends of a cell, or two cell terminals, with a metal tool, a wire or a nickel strip.
> - Do not use the soldering iron directly on the cells: heat damages them. Spot-weld nickel strips to the cells and solder the wires only to the nickel strips, **before** the strips are welded to the cells.
> - Work on a clean, non-conductive surface and check the polarity of every cell with the multimeter before you join it.
> - Insulate every exposed metal part before the pack is handled further.
>
> **Soldering fumes.** Solder in a ventilated place and do not breathe the fumes from the flux.

### Parts list

**Printed parts:** the 15 STL files and their quantities are in [models/cad/Stl/](../../models/cad/Stl/README.md).

**Electronics and other bought parts** (details and links in the [Bill of Materials](../../README.md#bom)):

| Part | Qty |
|---|---|
| Raspberry Pi 5 | 1 |
| Raspberry Pi Active Cooler | 1 |
| Raspberry Pi AI HAT+ 26 TOPS (Hailo-8), with its PCIe flat cable | 1 |
| GPIO stacking header (pin extender) | 1 |
| GPIO header spacer | 1 |
| microSD card | 1 |
| Raspberry Pi Camera Module 3 Wide and its flat cable | 1 |
| Slamtec RPLidar S2L | 1 |
| Hiwonder ROS Robot Controller | 1 |
| Hiwonder JGB37-520 drive motor | 1 |
| Hiwonder HX-12H bus servo | 1 |
| Panasonic NCR18650B cells | 3 |
| 3S BMS | 1 |
| Interlocking 18650 spacer brackets (generic) | 4 |
| Grove LED button | 1 |
| LEGO tyres 62.4 × 20 S with rims | 4 |

**LEGO Technic parts** (also in the [Bill of Materials](../../README.md#bom)):

| Part | LEGO part number | Qty |
|---|---|---|
| Differential housing and 28-tooth bevel gear | 65414, 65413 | 1 each |
| Axle 5.5L with stop | 32209 | 3 |
| Axle 4L with stop | 87083 | 2 |
| Pin connector round 2L with slot | 62462 | 2 |
| Axle connector 2L, smooth | 6538c | 1 |

**Screws, standoffs and heat-set inserts.** These come in the kits and packs listed in the [Bill of Materials](../../README.md#bom); the robot needs only the quantities below.

| Part | Size | Qty |
|---|---|---|
| Heat-set inserts | M2.5, 4 mm long | 18 |
| Heat-set inserts | M2, 2 mm long | 2 |
| Screws | M2.5 × 4 | 6 |
| Screws | M2.5 × 6 | 4 |
| Screws | M2.5 × 8 | 4 |
| Screws | M3 × 6 | 5 |
| Screws | M3 × 8 | 6 |
| Screws | M3 × 10 | 4 |
| Standoffs, male–female | M2.5, 6 mm | 8 |
| Standoffs, male–female | M2.5, 10 mm | 2 |
| Standoffs, female–female | M2.5, 15 mm | 4 |

---

## 1. Battery pack

### Step 1: Hold the three cells together

<img src="v7/IMG_3634.jpg" alt="Two 18650 cells end to end in interlocking spacer brackets and the third cell beside them in the printed battery holder" width="480">

Put two Panasonic NCR18650B cells end to end in four interlocking 18650 spacer brackets. Put the third cell beside them in the printed holder (`battery_holder.STL`), so that all three cells are at the same height; this keeps the controller board low enough not to block the LiDAR. Check the polarity of every cell with the multimeter, then spot-weld nickel strips to connect the cells **in series** (3S). Solder the wires to the nickel strips **before** you weld the strips to the cells, never on the cells themselves. Read the battery safety warning above first.

### Step 2: Connect the 3S BMS

<img src="v7/IMG_3639.jpg" alt="Blue 3S protection board taped to the cells, with the balance wires and the XT60 power lead" width="480">

Solder the balance wires and the main leads to the 3S protection board (BMS), then solder the XT60 connector to the output leads. Keep the soldering iron away from the cells. Tape the board flat against the cells.

### Step 3: Insulate the pack

<img src="v7/IMG_3643.jpg" alt="Finished battery pack wrapped in black insulating tape, with the XT60 connector and the balance plug" width="480">

Wrap the whole pack in insulating tape so that no metal is exposed. Only the XT60 power connector and the balance plug come out. Measure the voltage: a charged pack reads about 12.6 V.

---

## 2. Raspberry Pi and AI HAT

### Step 4: Lay out the parts

<img src="v7/IMG_1885.jpg" alt="Raspberry Pi 5 with its cooler, AI HAT+ (Hailo-8), stacking header, brass standoffs, camera cable and microSD card" width="480">

You need the Raspberry Pi 5 with its Active Cooler, the AI HAT+ 26 TOPS (Hailo-8) with its PCIe flat cable, the GPIO stacking header and the GPIO header spacer, the brass standoffs and their screws, the camera flat cable and the microSD card with the operating system.

### Step 5: Fit the standoffs and the stacking header

<img src="v7/IMG_1887.jpg" alt="Side view of the Raspberry Pi with the brass standoffs and the stacking header fitted" width="480">

Fit the Active Cooler, screw the brass standoffs to the Raspberry Pi, and press the GPIO stacking header onto the GPIO pins with the header spacer.

### Step 6: Mount the AI HAT and connect the camera cable

<img src="v7/IMG_1891.jpg" alt="AI HAT+ mounted on the Raspberry Pi, with the camera flat cable connected" width="480">

Press the AI HAT onto the stacking header, connect its flat cable to the Pi's PCIe connector, and screw it to the standoffs. Connect the camera flat cable to the Pi. Check that both cable latches are closed.

---

## 3. Camera

### Step 7: Lay out the camera parts

<img src="v7/IMG_1899.jpg" alt="Printed camera mast, printed camera frame, Camera Module 3 Wide, a long screw and a nut" width="480">

You need the printed camera mast (`camera_mast.STL`), the printed camera frame (`camera_hinge.STL`), the Camera Module 3 Wide, one long screw and one nut.

### Step 8: Fit the camera in its frame

<img src="v7/IMG_1900.jpg" alt="Camera Module 3 Wide fitted in the printed frame" width="480">

Push the camera board into the frame with the lens through the opening.

### Step 9: Join the frame to the mast

<img src="v7/IMG_1903.jpg" alt="Camera frame joined to the bottom of the V-shaped mast with the long screw" width="480">

Put the frame's hinge between the two tabs at the bottom of the mast and pass the long screw through, then fit the nut. Tighten it just enough that the camera keeps its tilt.

---

## 4. Heat-set inserts

### Step 10: Fit the heat-set inserts

<img src="v7/IMG_1924.jpg" alt="Soldering iron pressing a brass heat-set insert into a printed part" width="480">

Press a brass heat-set insert into every insert hole of the printed parts (the holes are already in the STL files), with the hot soldering iron: M2.5 inserts, 4 mm long, in most holes and the two M2 inserts, 2 mm long, where the holes are smaller. Stop when the insert is level with the surface and let the plastic cool before you put a screw in. The LiDAR plate in the next section already needs them.

---

## 5. LiDAR plate

### Step 11: Lay out the plate parts

<img src="v7/IMG_1894.jpg" alt="Two printed plates, screws and the RPLidar S2L" width="480">

You need the two printed plates (the LiDAR plate, `hiwonder_lidar_support.STL`, and the Hiwonder board plate, `hiwonder_support.STL`), their screws and the RPLidar S2L.

### Step 12: Join the plates

<img src="v7/IMG_1895.jpg" alt="The two printed plates screwed together into one long plate with heat-set inserts" width="480">

Screw the two plates together into one long plate. The wide end carries the LiDAR; the narrow end, with the heat-set inserts, carries the controller board.

### Step 13: Mount the LiDAR

<img src="v7/IMG_1898.jpg" alt="RPLidar S2L screwed to the wide end of the plate" width="480">

Screw the LiDAR to the wide end of the plate, with its cable leaving towards the side.

---

## 6. Chassis and drivetrain

### Step 14: Lay out the drivetrain parts

<img src="v7/IMG_1905.jpg" alt="Printed chassis base with the LEGO differential gears, axles and bushes" width="480">

You need the chassis base (`chassis_base.STL`), the LEGO differential (housing 65414 and the 28-tooth gear 65413), the two rear axles and their bushes.

### Step 15: Put the differential in the gearbox

<img src="v7/IMG_1908.jpg" alt="LEGO differential with its red 28-tooth gear sitting in the rear gearbox of the chassis" width="480">

Place the differential in the gearbox at the rear of the chassis, with the red 28-tooth gear on the motor side.

### Step 16: Insert the first axle

<img src="v7/IMG_1913.jpg" alt="First rear axle pushed through the chassis wall into the differential, with its bushes" width="480">

Push the first axle through the side wall into the differential and slide its bushes on.

### Step 17: Stop the axle from sliding out

<img src="v7/IMG_1911.jpg" alt="A drop of glue being applied to a bush on the rear axle" width="480">

Put one small drop of instant glue where the photo shows, so that the axle cannot slide out. Keep the glue away from the differential and from the parts that must turn.

### Step 18: Insert the second axle

<img src="v7/IMG_1918.jpg" alt="Both rear axles through the differential" width="480">

Fit the second axle from the other side in the same way. Turn the axles by hand: they must turn freely and the differential must work.

### Step 19: Prepare the drive motor

<img src="v7/IMG_1919.jpg" alt="JGB37-520 drive motor, its 20-tooth gear and the screws next to the chassis" width="480">

You need the JGB37-520 drive motor, the printed 20-tooth gear (`motor_gear_20t.STL`) and the motor screws.

### Step 20: Mount the drive motor

<img src="v7/IMG_1921.jpg" alt="Drive motor screwed to the round mount at the rear of the chassis" width="480">

Screw the motor face to the round mount at the rear of the chassis.

### Step 21: Fit the motor gear

<img src="v7/IMG_1923.jpg" alt="20-tooth gear on the motor shaft meshing with the red 28-tooth differential gear" width="480">

Fit the printed 20-tooth gear on the motor shaft. Turn a rear wheel axle by hand and check that the 20:28 gears mesh without jamming.

---

## 7. Raspberry Pi on the chassis

### Step 22: Mount the Raspberry Pi

<img src="v7/IMG_1929.jpg" alt="Raspberry Pi with the AI HAT screwed to the chassis in front of the drive motor" width="480">

Screw the Raspberry Pi and AI HAT stack to the chassis, in front of the drive motor. Lead the camera cable out to the side; it goes to the camera later.

---

## 8. Steering

### Step 23: Mount the steering servo

<img src="v7/IMG_1931.jpg" alt="Steering bus servo screwed to the front of the chassis" width="480">

Screw the HX-12H steering servo to the front of the chassis, with its shaft facing up. Set the servo to its centre position (750) before you fit the arm.

### Step 24: Lay out the steering parts

<img src="v7/IMG_1932.jpg" alt="Printed steering link, servo arm, two wheel knuckles and small joint parts" width="480">

You need the steering link (`steering_link.STL`), the servo arm (`steering_servo_arm.STL`), the two wheel knuckles (`ackerman_wheel_join.STL` and its mirror) and the small joint parts.

### Step 25: Join the servo arm to the link

<img src="v7/IMG_1935.jpg" alt="Servo arm joined to the middle of the steering link" width="480">

Fix the servo arm to the middle of the steering link.

### Step 26: Fit the joints at both ends

<img src="v7/IMG_1936.jpg" alt="Steering link with the Ackermann joints fitted at both ends" width="480">

Fit an Ackermann joint at each end of the link. Each joint must swing freely.

### Step 27: Mount the steering on the chassis

<img src="v7/IMG_1937.jpg" alt="Steering assembly on the front of the chassis, with the servo arm on the servo horn" width="480">

Put the servo arm on the servo horn and screw it, with the wheels pointing straight ahead. Seat a wheel knuckle on each side of the front mount.

### Step 28: Check the front axle

<img src="v7/IMG_1940.jpg" alt="Front view of the steering: servo, knuckles and the two front stub axles" width="480">

Fit the two front stub axles in the knuckles. Turn the servo arm by hand to both sides: nothing may touch the chassis, and both wheels must steer together.

---

## 9. Battery and start button

### Step 29: Place the battery pack

<img src="v7/IMG_1941.jpg" alt="Taped battery pack mounted across the chassis between the Raspberry Pi and the steering" width="480">

Place the battery pack across the chassis between the Raspberry Pi and the steering, with the XT60 connector on the side. Do not connect it yet.

### Step 30: Mount the start button

<img src="v7/IMG_1942.jpg" alt="Side view of the chassis with the Grove LED button mounted on the side wall" width="480">

Screw the Grove LED button to the side wall of the chassis and connect its cable to the Raspberry Pi (button GPIO4, LED GPIO3). Check that the button can be pressed from outside.

---

## 10. LiDAR plate and controller board

### Step 31: Mount the LiDAR plate

<img src="v7/IMG_1944.jpg" alt="LiDAR plate screwed over the Raspberry Pi, with the LiDAR in the centre of the robot" width="480">

Screw the LiDAR plate onto the chassis so that the LiDAR sits in the centre of the robot and the narrow end of the plate points to the front. Connect the LiDAR to a USB port of the Raspberry Pi.

### Step 32: Mount the controller board

<img src="v7/IMG_1945.jpg" alt="Hiwonder controller board being screwed to the front end of the plate" width="480">

Screw the Hiwonder ROS Robot Controller to the narrow end of the plate.

### Step 33: Connect the controller board

<img src="v7/IMG_1948.jpg" alt="Controller board mounted at the front, with the battery lead in its power terminal" width="480">

Connect, as in the [connection table](../../README.md#connections): the battery lead to the board's power input, the drive motor to port M4, the steering servo to a bus servo port, the board's 5 V Type-C output to the Pi's power input, and a USB cable from the Pi to the board's USB serial port.

---

## 11. Camera mast and wheels

### Step 34: Mount the camera mast

<img src="v7/IMG_1949.jpg" alt="Camera mast being screwed to the rear of the chassis over the drive motor" width="480">

Screw the two legs of the camera mast to the rear of the chassis, over the drive motor, and connect the camera flat cable to the camera.

### Step 35: Check the robot before the wheels

<img src="v7/IMG_1951.jpg" alt="Top view of the robot without wheels: camera mast at the rear, LiDAR in the centre, controller board at the front" width="480">

Check every connection against the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png). Tie the cables so that none touches the gears, the axles or the LiDAR.

### Step 36: Fit the wheels

<img src="v7/IMG_1953.jpg" alt="Robot with all four wheels fitted" width="480">

Push the four LEGO 62.4 × 20 S wheels onto the axles. Turn each wheel by hand: the front wheels spin freely and the rear wheels turn the drive gears.

### Step 37: Finished robot

<img src="v7/IMG_1972.jpg" alt="Finished robot seen from above on the field" width="480">

Put the robot on a flat surface and check that all four wheels touch the ground. Before the first run, flash the board firmware and do the checks in [§9 of the README](../../README.md#setup-instructions).

---

## Final checks

- All four wheels turn freely, and the rear wheels drive the gears without jamming.
- The steering moves fully to both sides without touching the chassis.
- No cable touches the gears, the wheels or the LiDAR.
- The LiDAR has a clear view all around.
- The battery is fully insulated and its connector cannot touch metal.
