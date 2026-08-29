#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

inline int terminating_main = 0;

enum direction{
    right = 1,
    left = -1,
    front = 2,
    behind = -2,
    invalid = 0
};

enum Color_traffic_light{
    none = 0,
    light_red = 2,
    light_green = 1,
    light_xparking = 3
};

enum Cube_number{
    cube_first = 2000,
    cube_second = 1250

};

enum Cube_number_chr{
    CUBE_first = 1000,
    CUBE_second = 1500,
    CUBE_middle = 1450

};

enum Brake_type{
    Hold = 1,
    Coast = 2,
    No_brake = 3

};

// Lidar buffer angle index (0-359) for each side of the robot. Offset already
// confirmed empirically for this robot in object_detection.cpp (Select_Wall):
// right=0, left=180, front=270, behind=90 in the Oradar's raw buffer space.
enum LidarSide{
    FRONT = 0,
    RIGHT = 90,
    BACK = 180,
    LEFT = 270
};
#endif // _COMMON_VAR_H