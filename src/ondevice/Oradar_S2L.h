// Lidar interface of the Open Challenge program (also linked by the Obstacle
// Challenge program). The "Oradar_S2L" name comes from design v6 (Oradar
// MS200 lidar); in design v7 this code drives the Slamtec RPLidar S2L
// (1 Mbaud, 10 Hz, 360 deg). A background thread keeps a 360-entry buffer of
// distances in mm (one per degree); the functions below read it to drive
// along the walls, detect the turning direction and measure wall angles.

#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

#include "common_var.h"
#include <cstdint>
#include <math.h>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
#endif //_WIN32

// Mission code uses "RP" angles (from the original RPLidar_S2L driver):
// 0=front, 90=right, 180=back, 270=left (the LidarSide values of
// common_var.h). The shared 360-entry buffer keeps the layout of the design v6
// Oradar mounting: right=0, back=90, left=180, front=270, and both drivers in
// Oradar_S2L.cpp fill it that way. ORADAR_ANGLE_OFFSET is the rotation (deg)
// added to an RP angle to get the buffer index; the four values match a
// single +270 deg shift, as confirmed on the field in object_detection.cpp
// (Select_Wall).
#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

// Wall found by Oradar_S2L_Wall_Slope on one side, for the lidar viewer.
struct Oradar_S2L_Wall_Info{
    bool found = false;
    float distance_mm = 0.0f;      // perpendicular distance from the lidar to the wall
    float orientation_deg = 0.0f;  // wall line angle; Oradar_S2L_Wall_Slope returns 10 times this
    float centroid_x_mm = 0.0f;    // centroid of the wall points, RP frame
    float centroid_y_mm = 0.0f;
    std::vector<float> x_mm;       // wall points, "RP" frame
    std::vector<float> y_mm;
};

// Connects to the lidar (retries every 1 s until it answers or
// Oradar_S2L_Set_Terminating() is called), starts the motor and the scan.
void Oradar_S2L_Init_Lidar(void);
// pthread entry point: grabs full scans and writes the per-degree average
// distance (mm) into the shared buffer until Oradar_S2L_Set_Terminating().
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
float Oradar_S2L_Radians_To_Degrees(float radians);
float Oradar_S2L_Degrees_To_Radians(float degrees);

// Driving helpers. speed: drive speed in % (SPIKE scale, 0..100); reference:
// heading to hold in deg (gyro PD via Spike_Forward). They block, moving the
// robot, until the lidar condition is met or the program is terminating.

// Drives forward until a side opening (> 1350 mm, or no return) appears and
// the front wall is close (<= 1100 mm). Returns the side of the opening, i.e.
// the driving direction of the run (right = clockwise), or invalid.
direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference);
// Drives forward until the front wall is <= 1100 mm and either the left (or
// right) wall has disappeared (>= 1350 mm) or the front wall is <= 500 mm:
// the corner where the robot turns.
void Oradar_S2L_Advance_Until_Left_Gap(int speed, int reference);
void Oradar_S2L_Advance_Until_Right_Gap(int speed, int reference);
// Drive "degrees" motor degrees and return the wall angle (tenths of a deg)
// from the change of the side distance.
int Oradar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference);
int Oradar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference);
// Drive until the front distance is <= target_distance mm (_R: until it is
// >= target_distance mm), then hold the motors.
void Oradar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance);
void Oradar_S2L_Advance_Until_Distance_R(int speed, int reference, int target_distance);
// Wall angle (tenths of a deg) from two side rays "degree" deg apart (law of
// cosines).
int Oradar_S2L_Correction_For_Triangles_Right(int degree);
int Oradar_S2L_Correction_For_Triangles_Left(int degree);
// Picks a consistent value from three readings (30-unit agreement window).
int Oradar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c);
// Least-squares line slope (tenths of a deg) over the "point" one-degree
// rays just below RP angle middle_point.
int Oradar_S2L_Slope(const int point, const int middle_point);
// Angle (tenths of a deg) of the wall around RP angle middle_point, found by
// point-cloud segmentation; 0 if no wall is found.
int Oradar_S2L_Wall_Slope(int middle_point);
// Same wall as Oradar_S2L_Wall_Slope, with its points and distance, for the viewer.
Oradar_S2L_Wall_Info Oradar_S2L_Get_Wall_Info(int middle_point);
int Oradar_S2L_Average(int arg_1, int arg_2);
// Stops the scan and the lidar motor and releases the driver. Join the writer
// thread first.
void Oradar_S2L_Close(void);
// Asks the writer thread and every blocking loop to finish.
void Oradar_S2L_Set_Terminating(void);
// Copies the 360 distances (mm, 0 = no return yet) under the buffer mutex.
void Oradar_S2L_Get_Buffer(float *buffer);
// Number of full scans written so far (0 = no data yet).
unsigned long Oradar_S2L_Get_Scan_Seq(void);
// For the emergency stop: lidar serial port, and the raw frames that stop its
// motor, ready to be written from a signal handler. Oradar_S2L_Stop_Frames
// returns how many bytes it wrote into buf (0 if this lidar has none).
const char *Oradar_S2L_Port(void);
int Oradar_S2L_Stop_Frames(unsigned char *buf, int max);

#endif // _ORADAR_S2L_H
