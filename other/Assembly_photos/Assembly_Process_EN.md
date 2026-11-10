# Assembly instructions

**WRO Future Engineers 2026 · Team CSS · "The Maker"**

> Photos of the current electronics (Hiwonder controller, JGB37 motor, HX-35H servo, RPLidar S2L) will be added. Some photos below show an earlier controller, motors, steering drive and LiDAR on the same chassis; the order of assembly is the same.

## Before you start

- Print the parts. The STL files are in [models/cad/Stl/](../../models/cad/Stl/README.md). The team used a Creality K1C printer with a 0.4 mm nozzle, Creality Hyper ABS filament and the default slicer profile.
- Have a soldering iron ready. You will use it to press the brass heat-set inserts into the printed parts.
- Collect all the parts in the [Bill of Materials](../../README.md#construction-guide).
- Keep the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png) open. Use it for every electrical connection.

---

## Chassis and inserts

### Step 1: Check the printed chassis

<img src="IMG_E6620.JPG" alt="Black 3D-printed chassis on the work mat" width="480">

Remove any strings and support material from the chassis. Check that the base is flat and that every hole is clean.

### Step 2: Fit the heat-set inserts

<img src="IMG_E6631.JPG" alt="Soldering iron pressing a heat-set insert into the chassis" width="480">

Press a brass heat-set insert into each insert hole with the hot soldering iron. Stop when the insert is level with the surface, and let the plastic cool before you put a screw in.

---

## Drivetrain

### Step 3: Prepare the rear axle parts

<img src="IMG_E6634.JPG" alt="Two LEGO axles with bushes next to the LEGO differential" width="480">

Slide the bushes onto the two LEGO axles as shown. Keep the LEGO differential ready; its ring gear is the large gear of the 20:28 gear pair.

### Step 4: Place the differential

<img src="IMG_E6636.JPG" alt="Top view of the chassis with the differential in the rear gearbox and one axle pushed in" width="480">

Hold the differential inside the gearbox at the rear of the chassis. Push the first axle through the side wall and into the differential.

### Step 5: Insert the second axle

<img src="IMG_E6640.JPG" alt="Top view of the chassis with both rear axles through the differential" width="480">

Push the second axle in from the other side until it sits in the differential. Check that both axles turn freely.

### Step 6: Prepare the drive motor

<img src="IMG_E6642.JPG" alt="Drive motor with its gear fitted, lying next to the chassis" width="480">

Fit the small gear of the 20:28 pair on the shaft of the drive motor. Check that the gear is tight on the shaft.

### Step 7: Fit the drive motor

<img src="IMG_E6645.JPG" alt="Hand pressing the drive motor into the frame at the rear of the chassis" width="480">

Press the drive motor down into the frame above the rear axle until it sits flat. Turn one rear axle by hand and check that the gears mesh and turn smoothly.

---

## Electronics

### Step 8: Prepare the Raspberry Pi

<img src="IMG_E6647.JPG" alt="Raspberry Pi with the AI HAT fitted and the flat camera cable connected" width="480">

Mount the AI HAT on the Raspberry Pi 5 and connect the flat camera cable. Check that the cable latch is closed and that the cable does not pull out.

### Step 9: Fit the controller

<img src="IMG_E6652.JPG" alt="Controller placed at the rear of the chassis, behind the drive motor" width="480">

Place the controller at the rear of the chassis, behind the drive motor. Check that it sits flat and that you can still reach its connectors.

### Step 10: Fit the Raspberry Pi

<img src="IMG_E6658.JPG" alt="Hands sliding the Raspberry Pi into the chassis in front of the drive motor" width="480">

Slide the Raspberry Pi, standing on its edge, into the slot in front of the drive motor. Guide the camera cable out so that it is not trapped.

### Step 11: Check the layout from above

<img src="IMG_E6663.JPG" alt="Top view of the chassis: Raspberry Pi in front of the drive motor and controller at the rear" width="480">

Look at the chassis from above. The Raspberry Pi must stand between the steering mount and the drive motor, and you must be able to reach its USB and power ports.

### Step 12: Fit the start button

<img src="IMG_E6666.JPG" alt="Grove LED button mounted on the side of the drive motor frame" width="480">

Fix the Grove LED start button to the side of the drive motor frame and connect its cable as shown in the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png). Check that you can press the button from outside the robot.

### Step 13: Connect the drive motor

<img src="IMG_E6669.JPG" alt="Hands connecting and folding flat cables next to the controller" width="480">

Plug the drive motor cable into the controller. Fold the spare cable and keep it inside the chassis, away from the rear axle and the gears.

---

## Battery

### Step 14: Place the battery pack

<img src="IMG_E6675.JPG" alt="18650 battery pack resting against the side of the chassis, seen from above" width="480">

Bring the 18650 battery pack to its bay beside the drive motor, with the power cables pointing out. Do not connect the pack yet.

### Step 15: Seat the battery pack

<img src="IMG_E6677.JPG" alt="Hand pushing the battery pack into the side of the chassis" width="480">

Push the battery pack fully into its bay and lead the red and black power cables forward along the base. Check that the pack cannot slide out and that no cable is pinched.

---

## Front axle and steering

### Step 16: Prepare the steering parts

<img src="IMG_E6681.JPG" alt="Steering actuator with a pinion gear, steering rack with two links and stub axles, and two pins" width="480">

Fit the pinion gear on the shaft of the steering actuator. Join the steering rack to its two Ackermann links and to the two wheel joins with their stub axles.

### Step 17: Fit the steering rack

<img src="IMG_E6683.JPG" alt="Hands fitting the steering rack and the stub axles at the front of the chassis" width="480">

Lay the rack across the front of the chassis and seat one wheel join on each side of the front mount. Check that the rack slides to both sides.

### Step 18: Fit the steering actuator

<img src="IMG_E6684.JPG" alt="Top view of the front of the chassis with the steering actuator fitted and its pinion on the rack" width="480">

Mount the steering actuator at the very front of the chassis so that its pinion sits on the rack. Check that the teeth are fully engaged.

### Step 19: Centre the steering

<img src="IMG_E6688.JPG" alt="Close-up of the front axle with the pinion in the middle of the steering rack" width="480">

Set the rack so that the pinion is in the middle and both stub axles point straight out. Move the rack by hand to each end and check that nothing touches the chassis.

### Step 20: Tighten the steering pivots

<img src="IMG_E6690.JPG" alt="Screwdriver tightening the pivot screw of a front wheel join" width="480">

Tighten the pivot screw of each wheel join with a small screwdriver. The wheel join must have no play but must still swing freely.

---

## LiDAR and camera

### Step 21: Prepare the LiDAR

<img src="IMG_E6696.JPG" alt="LiDAR with its USB adapter, printed support plates and screws" width="480">

Get the LiDAR, its USB adapter, the printed LiDAR support and the screws ready. Check that the support has its heat-set inserts.

### Step 22: Fit the LiDAR support

<img src="IMG_E6700.JPG" alt="Printed LiDAR support fitted between the front axle and the Raspberry Pi" width="480">

Fix the LiDAR support to the chassis between the front axle and the Raspberry Pi. Check that it does not press on any cable.

### Step 23: Mount the LiDAR

<img src="IMG_E6702.JPG" alt="Hand placing the LiDAR on its support" width="480">

Place the LiDAR on the support and fix it with its screws. Lead its cable down the side, away from the steering rack.

### Step 24: Check the LiDAR

<img src="IMG_E6706.JPG" alt="Top view of the LiDAR mounted behind the front axle" width="480">

Check that the LiDAR sits flat and does not move. Connect it as shown in the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png).

### Step 25: Fit the camera mast

<img src="IMG_E6710.JPG" alt="Camera mast fitted over the drive motor, robot still without wheels" width="480">

Fix the two legs of the camera mast to the sides of the drive motor frame, with the camera at the top. Check that the mast does not wobble.

---

## Wiring check

### Step 26: Connect the cables to the controller

<img src="IMG_E6711.JPG" alt="Hand plugging a cable into the controller at the rear of the robot" width="480">

Plug the remaining motor and steering cables into the controller. Push each connector in until it is fully seated.

### Step 27: Connect the camera and check the wiring

<img src="IMG_E6712.JPG" alt="Robot without wheels seen from above, with all cables connected and the camera cable running up the mast" width="480">

Run the flat camera cable from the Raspberry Pi up the mast and plug it into the camera. Compare every connection with the [wiring diagram](../../schemes/hardware/wiringSchematics_v7.png) and keep all cables inside the chassis.

---

## Wheels

### Step 28: Prepare the wheels

<img src="IMG_E6714.JPG" alt="Four wheels with LEGO 62.4 x 20 S tyres and two yellow half bushes" width="480">

Check that each of the four LEGO 62.4 x 20 S tyres sits evenly on its wheel.

### Step 29: Fit the front wheels

<img src="IMG_E6715.JPG" alt="Two front wheels fitted, next to the steering actuator and the LiDAR" width="480">

Push one wheel onto each front stub axle. Check that each wheel spins freely and does not rub on the steering links.

### Step 30: Fit the rear wheels

<img src="IMG_E6717.JPG" alt="Hands fitting a rear wheel while the front wheels are already on" width="480">

Push one wheel onto each end of the rear axle. Turn one rear wheel by hand and check that the drive gears turn with it.

### Step 31: Stand the robot on its wheels

<img src="IMG_E6721.JPG" alt="Finished robot on its four wheels, seen from above" width="480">

Put the robot on a flat surface. Check that all four wheels touch the ground and that the robot rolls in a straight line when you push it.

---

## Final checks

- All four wheels turn freely.
- The steering moves fully to both sides without touching the chassis.
- No cable touches the gears or the wheels.
- The LiDAR has a clear view all around.
