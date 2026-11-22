// Shared types for the Open Challenge program (ondevice/): turn direction
// and the lidar angle indices of each side of the robot. Used by main.cpp,
// the lidar drivers (Oradar_S2L.cpp, RPLidar_S2L.cpp) and lidar_display.cpp.

#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

// Mounting of the lidar: the lidar's OWN angle (its 0 is the mark on the
// lidar, increasing clockwise seen from above) that points to the FRONT of the
// robot. Oradar_S2L.cpp uses it as RPLIDAR_FRONT_DEG to store every scan
// already rotated into the robot's frame; that is why LidarSide (below) is
// fixed. Lidar rotated 270 deg clockwise -> 90; counter-clockwise -> 270.
// CHECK: a box 30 cm in front of the robot appears at the TOP of the window
// and in the "front" distance printed at start-up. It can be overridden
// without editing this file: make LIDAR_FLAGS="-DRPLIDAR_FRONT_DEG=90".
#define Lidar_Angle_Orientation_fixed_to_robot   90

// Driving direction around the field, also passed as the steering sign to
// Spike_Turn_For_Degrees(): right = clockwise, left = counter-clockwise.
enum direction{
    right = 1,
    left = -1,
    invalid = 0
};

// Angles of the ROBOT (deg, 0-359: 0 = front, 90 = right, clockwise), not of
// the lidar: the mounting is already corrected when the buffer is filled, so
// these values do not change if the lidar is rotated (rotating them here too
// would apply the rotation twice). RP_TO_ORADAR_IDX() in Oradar_S2L.h converts
// them to the index of the shared 360-entry scan buffer. Uppercase names to
// avoid colliding with enum direction's lowercase right/left above (different
// meaning: those are Spike turn directions, not lidar angles).
enum LidarSide{
    FRONT = 0,
    RIGHT = 90,
    FRONT_RIGHT = 75,
    BACK = 180,
    LEFT = 270,
    FRONT_LEFT = 285
};

#endif // _COMMON_VAR_H