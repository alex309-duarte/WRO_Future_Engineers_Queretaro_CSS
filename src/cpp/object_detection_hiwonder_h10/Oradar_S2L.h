#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

// Lidar del reto 2. El driver y el buffer de 360 grados viven en
// src/ondevice/Oradar_S2L.cpp (RPLidar S2L por defecto u Oradar MS200, se
// elige al configurar con WRO_LIDAR), el mismo que probó el reto 1. Aquí sólo
// se declara lo que usa la misión, más Oradar_S2L_Advance_Until_Distance con
// tipo de freno (lidar_reto2.cpp).
//
// Orientación del buffer (la del Oradar, confirmada en pista en Select_Wall):
// derecha = 0, atrás = 90, izquierda = 180, frente = 270. RP_TO_ORADAR_IDX
// pasa de los índices lógicos FRONT/RIGHT/BACK/LEFT de common_var.h al buffer.

#include "common_var.h"

#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

void Oradar_S2L_Init_Lidar(void);
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
void Oradar_S2L_Close(void);
void Oradar_S2L_Set_Terminating(void);
void Oradar_S2L_Get_Buffer(float *buffer);
unsigned long Oradar_S2L_Get_Scan_Seq(void);
float Oradar_S2L_Radians_To_Degrees(float radians);
float Oradar_S2L_Degrees_To_Radians(float degrees);

void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake);

#endif // _ORADAR_S2L_H
