// Obstacle Challenge (reto 2): drive forward until the LiDAR reads a given
// distance straight ahead, then stop with the selected brake type, as in
// object_detection_original_h10/Oradar_S2L.cpp. Reads the LiDAR buffer of the
// Open Challenge driver (src/ondevice/Oradar_S2L.cpp: RPLidar S2L or Oradar
// MS200, the same driver the Open Challenge proved on the track).

#include "Oradar_S2L.h"
#include "spike.h"

#include <cstdio>
#include <unistd.h>

// Drives with Spike_Forward(vel, referencia) (speed in %, heading in deg)
// until the front LiDAR distance is <= distancia_objetivo (same unit as the
// LiDAR buffer, mm) or the program is terminating. Polls about every 1 ms.
// Brake: Hold or Brake -> motors hold; otherwise they coast. Blocks.
void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake){
    float lidar[360];
    Oradar_S2L_Get_Buffer(&lidar[0]);
    float distancia_frente = lidar[RP_TO_ORADAR_IDX(FRONT)];

    // 0 = no reading yet for that degree: keep driving, as in the original.
    while((terminating_main == 0) && ((distancia_frente > distancia_objetivo) || (distancia_frente == 0.0f))){
        Spike_Forward(vel, referencia);
        usleep(1000);
        Oradar_S2L_Get_Buffer(&lidar[0]);
        distancia_frente = lidar[RP_TO_ORADAR_IDX(FRONT)];
    }
    printf("exit distance = %f\n", distancia_frente);
    if((brake == Hold) || (brake == Brake)){
        Spike_Hold_Motors();
    }
    else{
        Spike_Coast_Motors();
    }
}
