#ifndef _ORADAR_S2L_H
#define _ORADAR_S2L_H

#include "common_var.h"
#include <cstdint>
#include <math.h>

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
#endif //_WIN32

// El RPLidar_S2L original usa 0=frente, 90=derecha, 270=izquierda. El Oradar
// esta montado con otra orientacion fisica; ORADAR_ANGLE_OFFSET es la rotacion
// (en grados) que hay que sumarle a un indice "espacio RPLidar" para obtener el
// indice real en el buffer de 360 del Oradar. Offset confirmado empiricamente
// en object_detection.cpp (Select_Wall): right=0, left=180, front=270, behind=90
// -- los 4 valores son consistentes con un unico shift de +270 aplicado al
// mapeo del RPLidar (frente=0,derecha=90,izquierda=270,atras=180).
#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

void Oradar_S2L_Init_Lidar(void);
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
float Oradar_S2L_Radianes_A_Grados(float radianes);
float Oradar_S2L_Grados_A_Radianes(float grados);
direction Oradar_S2L_Avanzar_Deteccion_Sentido_Lidar(int vel, int referencia);
void Oradar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(int vel, int referencia);
void Oradar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(int vel, int referencia);
int Oradar_S2L_Avanzar_Dos_Puntos_Izquierda(int vel, int grados, int referencia);
int Oradar_S2L_Avanzar_Dos_Puntos_Derecha(int vel, int grados, int referencia);
void Oradar_S2L_Avanzar_Hasta_La_Distancia(int vel, int referencia, int distancia_objetivo);
int Oradar_S2L_Correction_For_Triangles_Right(int degree);
int Oradar_S2L_Correction_For_Triangles_Left(int degree);
int Oradar_S2L_Comparation(int arg1, int arg2, int arg3);
void Oradar_S2L_Close(void);
void Oradar_S2L_Set_Terminating(void);
void Oradar_S2L_Get_Buffer(float *buffer);
unsigned long Oradar_S2L_Get_Scan_Seq(void);

#endif // _ORADAR_S2L_H
