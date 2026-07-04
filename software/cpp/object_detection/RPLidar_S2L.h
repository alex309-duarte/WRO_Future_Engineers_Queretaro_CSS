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

struct temp_distance_struct{
    float  distance;
    int repetitions;
};

void signal_handler(int signum);
void RPLidar_S2L_Init_Lidar(void);
void *RPLidar_S2L_Lidar_Writer_Thread(void *arg);
float RPLidar_S2L_Radianes_A_Grados(float radianes);
float RPLidar_S2L_Grados_A_Radianes(float grados);
direction RPLidar_S2L_Avanzar_Deteccion_Sentido_Lidar(int vel, int referencia);
void RPLidar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(int vel, int referencia);
void RPLidar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(int vel, int referencia);
int RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(int vel, int grados, int referencia);
int RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(int vel, int grados, int referencia);
void RPLidar_S2L_Avanzar_Hasta_La_Distancia(int vel, int referencia, int distancia_objetivo);
int RPLidar_S2L_Correction_For_Triangles_Right(int degree);
int RPLidar_S2L_Correction_For_Triangles_Left(int degree);
int RPLidar_S2L_Comparation(int arg1, int arg2, int arg3);
void RPLidar_S2L_Close(void);
void RPLidar_S2L_Set_Terminating(void);
void RPLidar_S2L_Get_Buffer(float *buffer);
unsigned long RPLidar_S2L_Get_Scan_Seq(void);

#endif // _RPLIDAR_S2L_H