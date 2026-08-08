#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

enum direction{
    right = 1,
    left = -1,
    invalid = 0
};

// Lidar buffer angle index (0-359) for each side of the robot, relative to
// the lidar's own zero reference. Uppercase names to avoid colliding with
// enum direction's lowercase right/left above (different meaning: those are
// Spike turn directions, not lidar angles).
enum LidarSide{
    FRONT = 0,
    RIGHT = 90,
    BACK = 180,
    LEFT = 270
};

#endif // _COMMON_VAR_H