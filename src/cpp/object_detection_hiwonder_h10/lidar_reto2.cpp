// Reto 2: avance hasta una distancia del frente con tipo de freno, como en
// object_detection_original_h10/Oradar_S2L.cpp, sobre el buffer del lidar del
// reto 1 (src/ondevice/Oradar_S2L.cpp: RPLidar S2L u Oradar MS200, el mismo
// driver que el reto 1 probó en pista).

#include "Oradar_S2L.h"
#include "spike.h"

#include <cstdio>
#include <unistd.h>

void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake){
    float lidar[360];
    Oradar_S2L_Get_Buffer(&lidar[0]);
    float distancia_frente = lidar[RP_TO_ORADAR_IDX(FRONT)];

    // 0 = ese grado aún no tiene lectura: se sigue, como en el original.
    while((terminating_main == 0) && ((distancia_frente > distancia_objetivo) || (distancia_frente == 0.0f))){
        Spike_Forward(vel, referencia);
        usleep(1000);
        Oradar_S2L_Get_Buffer(&lidar[0]);
        distancia_frente = lidar[RP_TO_ORADAR_IDX(FRONT)];
    }
    printf("distancia a la cual sale = %f\n", distancia_frente);
    if((brake == Hold) || (brake == Brake)){
        Spike_Hold_Motors();
    }
    else{
        Spike_Coast_Motors();
    }
}
