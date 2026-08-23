#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

#include "common_var.h"

void Oradar_S2L_Init_Lidar(void);
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
void Oradar_S2L_Close(void);
void Oradar_S2L_Set_Terminating(void);
void Oradar_S2L_Get_Buffer(float *buffer);
float Oradar_S2L_Radianes_A_Grados(float radianes);
float Oradar_S2L_Grados_A_Radianes(float grados);
unsigned long Oradar_S2L_Get_Scan_Seq(void);
void Oradar_S2L_Avanzar_Hasta_La_Distancia(int vel, int referencia, int distancia_objetivo,Brake_type brake);

#endif // _ORADAR_S2L_H
