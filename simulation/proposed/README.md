# Proposed changes to the mission

Status: **tried in the digital twin only. Not tested on the robot.** The patch is not applied to the mission code; the twin builds a patched copy of it (`-Adapted` in `tools/run_episode.ps1`).

`mission_changes.patch` applies to `src/cpp/object_detection_hiwonder_h10/object_detection.cpp` (v2.0.3):

```bash
git apply --check simulation/proposed/mission_changes.patch
git apply simulation/proposed/mission_changes.patch
```

It holds two changes, each with its own switch:

| Switch | Change | Value 0 |
| --- | --- | --- |
| `MIDDLE_SIGN_MAKE_ROOM` | New manoeuvre for a single sign in the middle of a section | Previous manoeuvre |
| `CORNER_COLOUR_SETTLE_MS` (200) | Hold the robot before reading the colour of the corner sign | Read on the move, as before |

## 1. Middle sign right after a corner

### The case

A section has a single sign, there was no sign in the corner, and the last sign of the previous section was passed on the inner side. `Corner_Case` then turns late and the robot comes out of the corner about 1150 mm into the new section, near the middle of the lane. A sign in the middle position (1500 mm) is only about 300 mm in front of the bumper.

`esquivar_cubos_middle` turns about 60 degrees, advances `hypotenuse - 450` and turns back. From that position there is not enough room: when the sign is on the inner row (always the case in the parking section) the robot touches it.

In the twin, Layout A clockwise, the red sign of the parking section is passed on lap 1 (the robot comes out of the parking lot, a different start) and moved on lap 2.

### The change

`Esquivar_Cubo_Medio_Con_Espacio(outer_side, parking)` is called from `Desicion` and `Desicion_counterclockwise` in place of `esquivar_cubos_middle` / `esquivar_cubos_middle_counterclockwise`:

1. **Make room.** Reverse along the section until the LiDAR is 600 mm from the rear wall (3000 mm minus the fitted front wall, the measurement the mission already uses; the camera mast hides the rear wall from the LiDAR).
2. **Read the colour again**, with the whole sign in view. If it is not seen, the colour read before reversing stands.
3. **Change lane on a measurement.** Turn 45 degrees towards the lane and drive that diagonal until the LiDAR ray that is perpendicular to the outer wall reads the lane, instead of driving a computed distance. The ray index follows the gyro heading, so it reads the lateral position whatever the heading. The last 60 mm are driven slowly, because the LiDAR gives one reading every 100 ms.
4. **Steer back** to heading 0.
5. **Drive along the lane** to the point where the previous manoeuvre ended (LiDAR 1700 mm from the rear wall), passing the sign, so the corner case that follows starts from the same place as before.

Safeguards:

- If the robot is not where this case expects it (more than 1400 mm from the rear wall, two rear readings that disagree, or a lateral reading outside 100 to 900 mm), the previous manoeuvre runs instead.
- Nothing moves if the robot is already within 60 mm of the lane.
- The reverse is never longer than the distance measured before moving, and never longer than 800 mm.
- The diagonal is never longer than the lateral distance asks for, so without LiDAR confirmation it ends by distance in about the same place.
- A reading that jumps more than 120 mm (the ray crossing a parking limit) is skipped for at most 300 ms.
- A lane less than 220 mm away uses a 25 degree diagonal.

| Constant | Value | Meaning |
| --- | --- | --- |
| `MIDDLE_REAR_START_MM` | 600 | LiDAR to rear wall where the lane change starts |
| `MIDDLE_LANE_INNER_MM` | 775 | Inner lane, LiDAR to the outer wall |
| outer lane | 185 (400 in the parking section) | As in `calculte_angle_section_start_*_chr` |
| `MIDDLE_LANE_TURN_DEG` | 45 | Heading of the diagonal |
| `MIDDLE_LANE_LEAD_MM` | 65 | Lateral distance still gained while steering back |
| `MIDDLE_LANE_SLOW_MM` | 60 | Last part of the diagonal driven slowly |
| `MIDDLE_END_REAR_MM` | 1700 | LiDAR to rear wall where the manoeuvre ends |

## 2. Hold before the colour of the corner sign is read

`Corner_Case` reads `traffic_lights.light_color` the moment the LiDAR reports the reading point, with the robot still moving at about 0.6 m/s. When the section the robot is leaving has a sign on its last position, that sign is beside the camera by then, but the newest processed frame was taken about 0.1 s earlier and can still show it at the edge of the image, larger than the sign in the corner. The corner is then read with the colour of the sign just passed.

The change waits 200 ms with the motor held before the read, in `Corner_Case` and `Corner_Case_counterclockwise`. Cost: 0.2 s per corner, 2.4 s per run.

This one is a precaution. The wrong reads seen in the twin had a larger cause, a camera pipeline running behind (see the README of the twin); after that was corrected the runs were made with the hold in place, and the patch has not been run without it.

## Results in the twin

Layout A is the layout the changes were developed on. Clockwise, three laps, one run per row unless noted. "Clean" means every sign passed on the correct side and none touched.

| Layout A, clockwise | Mission as in the repository | Mission with the patch |
| --- | --- | --- |
| Plain layout | Lap 1 clean. Lap 2: the red sign of the parking section is hit and pushed away; the run does not recover (3 signs moved, 1 passed on the wrong side) | 3 laps clean, 15 of 15 signs, in 1 min 54.6 s |
| One sign replaced (surprise rule, 4 runs: 150 mm and 50 mm, red and green) | not run | 4 of 4 runs: 3 laps clean, modified sign passed on the side of its original colour 3 of 3 times |

Layout A counter-clockwise is **not solved by either version**: both stop against a wall at the corner after the parking section, and one green sign is passed on the wrong side (on lap 1 with the patch, on lap 2 without). The new manoeuvre was not called in that run. The counter-clockwise branch of the patch has therefore not been exercised in its final form.

The other four test layouts, clockwise, one run each:

| Layout | Mission as in the repository | Mission with the patch |
| --- | --- | --- |
| B | 0.95 laps: a sign touched at 11 s, then stopped against a wall | 2.96 laps: 12 passes correct, 7 wrong, 2 signs touched |
| C | 1.23 laps: 2 signs touched, then stopped against a wall | 2.95 laps: 13 passes correct, 9 wrong, none touched |
| D | Stopped against a wall in the first section (0.23 laps) | The same |
| E | 0.95 laps clean, then stopped against a wall | The same |

**Layouts B to E are not solved in the twin by either version.** It is not known yet whether the fault is in the twin (the start out of the parking lot and the turning radius are not calibrated against the robot) or in the mission. These rows are not results of the robot.

The final parking does not complete in the twin either (wall contacts after the third lap, with and without the patch).

## What to check on the robot before using it

- **The inner lane (775 mm) and the lead (65 mm).** The window is narrow: in the twin the rear axle has to end between about 720 mm (below it the robot touches an inner-row sign) and about 785 mm from the outer wall (from there the corner that follows touches a sign on the outer row of the next section). With these values the rear axle ended between 719 and 782 mm. `calculte_angle_section_start_*_chr` aims at 815 mm, but the manoeuvre it feeds ends near 750 mm.
- **The colour read after reversing**, at about 900 mm from the sign.
- **Counter-clockwise, red sign in the parking section.** The patch uses the 400 mm lane there. The previous code calls `calculte_angle_section_start_counterclockwise_chr` without the `parking` argument for red, so it used 185 mm.
- **Time.** The new manoeuvre costs about 4 s per middle-sign case (reverse, pause, colour read, slow end of the diagonal).
- The turning radius of the simulated car is not calibrated against the real robot. The lane change was designed not to depend on it (it ends on a wall distance), but it has only been run with one steering ratio.
