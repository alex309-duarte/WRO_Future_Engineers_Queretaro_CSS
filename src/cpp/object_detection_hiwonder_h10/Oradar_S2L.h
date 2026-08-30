#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

#include "common_var.h"

// El Oradar esta montado con otra orientacion fisica que la que asumen los
// indices FRONT/RIGHT/BACK/LEFT de LidarSide; ORADAR_ANGLE_OFFSET es la
// rotacion (en grados) que hay que sumarle a uno de esos indices logicos para
// obtener el indice real en el buffer de 360 del Oradar. Offset confirmado
// empiricamente mas abajo en este mismo proyecto (Select_Wall en
// object_detection.cpp): right=0, left=180, front=270, behind=90.
#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

void Oradar_S2L_Init_Lidar(void);
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
void Oradar_S2L_Close(void);
void Oradar_S2L_Set_Terminating(void);
void Oradar_S2L_Get_Buffer(float *buffer);
float Oradar_S2L_Radians_To_Degrees(float radians);
float Oradar_S2L_Degrees_To_Radians(float degrees);
unsigned long Oradar_S2L_Get_Scan_Seq(void);
void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake);

direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference);
void Oradar_S2L_Advance_Until_Left_Gap(int speed, int reference);
void Oradar_S2L_Advance_Until_Right_Gap(int speed, int reference);
int Oradar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference);
int Oradar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference);
int Oradar_S2L_Correction_For_Triangles_Right(int degree);
int Oradar_S2L_Correction_For_Triangles_Left(int degree);
int Oradar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c);
int Oradar_S2L_Slope(const int point, const int middle_point);
int Oradar_S2L_Slope2(const int point, const int middle_point);
int Oradar_S2L_Average(int arg_1, int arg_2);

#endif // _ORADAR_S2L_H
