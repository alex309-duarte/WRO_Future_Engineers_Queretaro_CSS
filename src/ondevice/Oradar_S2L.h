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

// El RPLidar_S2L original usa 0=frente, 90=derecha, 270=izquierda. El Oradar
// esta montado con otra orientacion fisica; ORADAR_ANGLE_OFFSET es la rotacion
// (en grados) que hay que sumarle a un indice "espacio RPLidar" para obtener el
// indice real en el buffer de 360 del Oradar. Offset confirmado empiricamente
// en object_detection.cpp (Select_Wall): right=0, left=180, front=270, behind=90
// -- los 4 valores son consistentes con un unico shift de +270 aplicado al
// mapeo del RPLidar (frente=0,derecha=90,izquierda=270,atras=180).
#define ORADAR_ANGLE_OFFSET 270
#define RP_TO_ORADAR_IDX(rp_idx) ((((rp_idx) + ORADAR_ANGLE_OFFSET) % 360 + 360) % 360)

// Pared detectada por Oradar_S2L_Wall_Slope en un lado, para visualizarla.
struct Oradar_S2L_Wall_Info{
    bool found = false;
    float distance_mm = 0.0f;      // perpendicular del robot a la pared
    float orientation_deg = 0.0f;  // Oradar_S2L_Wall_Slope devuelve 10 veces esto
    float centroid_x_mm = 0.0f;
    float centroid_y_mm = 0.0f;
    std::vector<float> x_mm;       // puntos de la pared, sistema "RP"
    std::vector<float> y_mm;
};

void Oradar_S2L_Init_Lidar(void);
void *Oradar_S2L_Lidar_Writer_Thread(void *arg);
float Oradar_S2L_Radians_To_Degrees(float radians);
float Oradar_S2L_Degrees_To_Radians(float degrees);
direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference);
void Oradar_S2L_Advance_Until_Left_Gap(int speed, int reference);
void Oradar_S2L_Advance_Until_Right_Gap(int speed, int reference);
int Oradar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference);
int Oradar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference);
void Oradar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance);
void Oradar_S2L_Advance_Until_Distance_R(int speed, int reference, int target_distance);
int Oradar_S2L_Correction_For_Triangles_Right(int degree);
int Oradar_S2L_Correction_For_Triangles_Left(int degree);
int Oradar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c);
int Oradar_S2L_Slope(const int point, const int middle_point);
int Oradar_S2L_Wall_Slope(int middle_point);
Oradar_S2L_Wall_Info Oradar_S2L_Get_Wall_Info(int middle_point);
int Oradar_S2L_Average(int arg_1, int arg_2);
void Oradar_S2L_Close(void);
void Oradar_S2L_Set_Terminating(void);
void Oradar_S2L_Get_Buffer(float *buffer);
unsigned long Oradar_S2L_Get_Scan_Seq(void);
// Para la parada de emergencia: puerto del lidar y las tramas que paran su
// motor, listas para escribirlas en crudo desde un manejador de señal.
// Devuelve cuántos bytes escribió en buf (0 si este lidar no las tiene).
const char *Oradar_S2L_Port(void);
int Oradar_S2L_Stop_Frames(unsigned char *buf, int max);

#endif // _ORADAR_S2L_H
