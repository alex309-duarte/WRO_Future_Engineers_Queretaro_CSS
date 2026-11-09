// Original Slamtec RPLidar S2L driver of the Open Challenge program, written
// on the Slamtec SDK (sl_lidar). Its 360-entry buffer uses the lidar's native
// angles: 0=front, 90=right, 180=back, 270=left (the LidarSide values of
// common_var.h). main.cpp now uses the Oradar_S2L interface, whose RPLidar
// S2L branch replaces this driver; the same mission helpers are kept here.

#ifndef _RPLIDAR_S2L_H
#define _RPLIDAR_S2L_H

#include "common_var.h"
#include <cstdint>
#include <math.h>
#include <sl_lidar.h>
#include <sl_lidar_driver.h>
// Number of elements of a static array.
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif // _countof

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
#endif //_WIN32

// Per-degree accumulator used to average all returns of one scan.
struct DistanceAccumulator{
    float distance_sum;
    int sample_count;
};

void signal_handler(int signum);
// Connects at 1 Mbaud on /dev/ttyUSB0, starts the motor and the scan.
void RPLidar_S2L_Init_Lidar(void);
// pthread entry point: fills the buffer with per-degree average distances (mm).
void *RPLidar_S2L_Lidar_Writer_Thread(void *arg);
float RPLidar_S2L_Radians_To_Degrees(float radians);
float RPLidar_S2L_Degrees_To_Radians(float degrees);
// Mission helpers with the same meaning as the Oradar_S2L_* functions:
// speed in %, reference = heading to hold in deg, distances in mm, angles
// returned in tenths of a degree.
direction RPLidar_S2L_Advance_And_Detect_Side(int speed, int reference);
void RPLidar_S2L_Advance_Until_Left_Gap(int speed, int reference);
void RPLidar_S2L_Advance_Until_Right_Gap(int speed, int reference);
int RPLidar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference);
int RPLidar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference);
void RPLidar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance);
int RPLidar_S2L_Correction_For_Triangles_Right(int degree);
int RPLidar_S2L_Correction_For_Triangles_Left(int degree);
int RPLidar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c);
// Wall angle from the "point" rays just below angle middle_point:
// ordinary least squares (Slope) or orthogonal regression (Slope2).
int RPLidar_S2L_Slope(const int point, const int middle_point);
int RPLidar_S2L_Slope2(const int point, const int middle_point);
int RPLidar_S2L_Average(int arg_1, int arg_2);
// Stops the scan and the motor and releases the driver.
void RPLidar_S2L_Close(void);
// Asks the writer thread and every blocking loop to finish.
void RPLidar_S2L_Set_Terminating(void);
// Copies the 360 distances (mm) into buffer.
void RPLidar_S2L_Get_Buffer(float *buffer);

#endif // _RPLIDAR_S2L_H
