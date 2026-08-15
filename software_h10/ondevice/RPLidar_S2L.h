#ifndef _RPLIDAR_S2L_H
#define _RPLIDAR_S2L_H

#include "common_var.h"
#include <cstdint>
#include <math.h>
#include <sl_lidar.h>
#include <sl_lidar_driver.h>
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif // _countof

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
#endif //_WIN32

struct DistanceAccumulator{
    float distance_sum;
    int sample_count;
};

void signal_handler(int signum);
void RPLidar_S2L_Init_Lidar(void);
void *RPLidar_S2L_Lidar_Writer_Thread(void *arg);
float RPLidar_S2L_Radians_To_Degrees(float radians);
float RPLidar_S2L_Degrees_To_Radians(float degrees);
direction RPLidar_S2L_Advance_And_Detect_Side(int speed, int reference);
void RPLidar_S2L_Advance_Until_Left_Gap(int speed, int reference);
void RPLidar_S2L_Advance_Until_Right_Gap(int speed, int reference);
int RPLidar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference);
int RPLidar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference);
void RPLidar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance);
int RPLidar_S2L_Correction_For_Triangles_Right(int degree);
int RPLidar_S2L_Correction_For_Triangles_Left(int degree);
int RPLidar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c);
int RPLidar_S2L_Slope(const int point, const int middle_point);
int RPLidar_S2L_Slope2(const int point, const int middle_point);
int RPLidar_S2L_Average(int arg_1, int arg_2);
void RPLidar_S2L_Close(void);
void RPLidar_S2L_Set_Terminating(void);
void RPLidar_S2L_Get_Buffer(float *buffer);

#endif // _RPLIDAR_S2L_H
