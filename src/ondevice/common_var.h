// Shared types for the Open Challenge program (ondevice/): turn direction
// and the lidar angle indices of each side of the robot. Used by main.cpp,
// the lidar drivers (Oradar_S2L.cpp, RPLidar_S2L.cpp) and lidar_display.cpp.

#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

// Driving direction around the field, also passed as the steering sign to
// Spike_Turn_For_Degrees(): right = clockwise, left = counter-clockwise.
enum direction{
    right = 1,
    left = -1,
    invalid = 0
};

// Logical lidar angle (deg, 0-359) for each side of the robot: 0 = front,
// clockwise. RP_TO_ORADAR_IDX() in Oradar_S2L.h converts it to the index of
// the shared 360-entry scan buffer. Uppercase names to avoid colliding with
// enum direction's lowercase right/left above (different meaning: those are
// Spike turn directions, not lidar angles).
enum LidarSide{
    FRONT = 0,
    RIGHT = 90,
    FRONT_RIGHT = 75,
    BACK = 180,
    LEFT = 270,
    FRONT_LEFT = 285
};

#endif // _COMMON_VAR_H