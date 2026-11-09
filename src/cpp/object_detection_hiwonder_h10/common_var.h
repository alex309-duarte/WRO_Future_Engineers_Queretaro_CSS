// Shared types of the Obstacle Challenge program: termination flag, sides,
// traffic-sign (pillar) colours as detected by the camera model, pillar
// positions, brake modes and the logical lidar angles.

#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

// Set to 1 by the Ctrl+C handler and read by every thread to finish
// (sig_atomic_t: safe to write from a signal handler).
inline volatile sig_atomic_t terminating_main = 0;

// Turn direction (right / left) and side of the robot for wall queries.
enum direction{
    right = 1,
    left = -1,
    front = 2,
    behind = -2,
    invalid = 0
};

// Class IDs of the YOLOv8n model (Hailo-8): green pillar, red pillar and the
// magenta parking-lot marker; none = nothing detected.
enum Color_traffic_light{
    none = 0,
    light_red = 2,
    light_green = 1,
    light_xparking = 3
};

// Pillar positions along a section in mm, used by the start-of-section
// geometry: Cube_number by avoid_cube_start_section(), Cube_number_chr by
// calculte_angle_section_start_*_chr() (first, second and middle pillar
// position).
enum Cube_number{
    cube_first = 2000,
    cube_second = 1250

};

enum Cube_number_chr{
    CUBE_first = 1000,
    CUBE_second = 1300,
    CUBE_middle = 1450

};

// How the drive motor stops at the end of a move.
enum Brake_type{
    Hold = 1,
    Coast = 2,
    No_brake = 3,
    Brake = 4

};

// Logical lidar angle (deg, 0 = front, clockwise) for each side of the robot.
// RP_TO_ORADAR_IDX (Oradar_S2L.h) converts it to the buffer index; the
// offset was confirmed for this robot in object_detection.cpp (Select_Wall):
// right=0, left=180, front=270, behind=90 in the raw buffer space.
enum LidarSide{
    FRONT = 0,
    RIGHT = 90,
    BACK = 180,
    LEFT = 270
};
#endif // _COMMON_VAR_H
