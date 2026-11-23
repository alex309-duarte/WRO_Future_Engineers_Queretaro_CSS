// Obstacle Challenge (reto 2): drive forward until the LiDAR reads a given
// distance straight ahead, then stop with the selected brake type, as in
// object_detection_original_h10/Oradar_S2L.cpp. Reads the LiDAR buffer of the
// Open Challenge driver (src/ondevice/Oradar_S2L.cpp: RPLidar S2L or Oradar
// MS200, the same driver the Open Challenge proved on the track).

#include "Oradar_S2L.h"
#include "spike.h"

#include <cstdio>
#include <unistd.h>

// Defined in object_detection.cpp (same executable).
float Distance_To_Wall(const direction side);

// Drives with Spike_Forward(vel, referencia) (speed in %, heading in deg)
// until the distance to the front wall is <= distancia_objetivo (mm) or the
// program is terminating. Polls about every 1 ms. Since 2026-11-22 the distance
// is the fitted front wall (Distance_To_Wall(front)) instead of the single ray
// straight ahead, so a pillar in front no longer stops the robot early.
// Used by both driving directions.
// Brake: Hold or Brake -> motors hold; otherwise they coast. Blocks.
void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake){
    float distancia_frente = Distance_To_Wall(front);

    // 0 = no wall found yet: keep driving, as in the original.
    while((terminating_main == 0) && ((distancia_frente > distancia_objetivo) || (distancia_frente == 0.0f))){
        Spike_Forward(vel, referencia);
        usleep(1000);
        distancia_frente = Distance_To_Wall(front);
    }
    printf("exit distance = %f\n", distancia_frente);
    if((brake == Hold) || (brake == Brake)){
        Spike_Hold_Motors();
    }
    else{
        Spike_Coast_Motors();
    }
}
