#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

// Lidar interface of the Obstacle Challenge program. The driver and the
// 360-degree buffer live in src/ondevice/Oradar_S2L.cpp (RPLidar S2L by
// default or Oradar MS200, chosen at configure time with WRO_LIDAR), the same
// driver proven in the Open Challenge. Only what the mission uses is declared
// here, plus Oradar_S2L_Advance_Until_Distance with a brake type
// (lidar_reto2.cpp).
//
// Buffer layout (the Oradar one, confirmed on the field in Select_Wall):
// right = 0, back = 90, left = 180, front = 270, distances in mm.
// RP_TO_ORADAR_IDX converts the logical angles FRONT/RIGHT/BACK/LEFT of
// common_var.h (0 = front, clockwise) to the buffer index.

#include "common_var.h"

#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

// Connects to the lidar and starts scanning (retries until it answers).
void Oradar_S2L_Init_Lidar(void);
// pthread entry point that keeps the buffer updated (one average per degree).
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
// Stops the lidar and releases the driver (join the writer thread first).
void Oradar_S2L_Close(void);
// Asks the writer thread to finish.
void Oradar_S2L_Set_Terminating(void);
// Copies the 360 distances (mm, 0 = no return yet) under the buffer mutex.
void Oradar_S2L_Get_Buffer(float *buffer);
// Number of full scans received so far (0 = no data yet).
unsigned long Oradar_S2L_Get_Scan_Seq(void);
float Oradar_S2L_Radians_To_Degrees(float radians);
float Oradar_S2L_Degrees_To_Radians(float degrees);

// Drives at vel % holding heading "referencia" (deg) until the front distance
// is <= distancia_objetivo mm, then stops with "brake" (blocks).
void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake);

#endif // _ORADAR_S2L_H
