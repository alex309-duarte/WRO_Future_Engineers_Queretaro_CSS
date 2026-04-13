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
void init_lidar(void);
void *lidar_writer_thread(void *arg);
float radianes_a_grados(float radianes);
direction avanzar_deteccion_sentido_lidar(int vel, int referencia);
void avanzar_deteccion_vacio_izquierdo_lidar(int vel, int referencia);
void avanzar_deteccion_vacio_derecho_lidar(int vel, int referencia);
int avanzar_dos_puntos_izquierda(int vel, int grados, int referencia);
int avanzar_dos_puntos_derecha(int vel, int grados, int referencia);
void avanzar_hasta_la_distancia(int vel, int referencia, int distancia_objetivo);
void RPLidar_S2L_Close(void);
void rplidar_s2l_set_terminating(void);
void RPLidar_S2L_Get_Buffer(float *buffer);

#endif // _RPLIDAR_S2L_H