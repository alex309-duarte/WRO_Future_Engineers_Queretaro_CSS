#ifndef SPIKE_H
#define SPIKE_H

// Reto 2 (obstacle challenge) sobre la placa Hiwonder: la misma API que usaba
// object_detection_original_h10 con el hub SPIKE, respaldada por la capa que
// el reto 1 ya probó en pista (src/ondevice): spike_compat + spike_hiwonder.cpp
// + hiwonder_config.h + parada_emergencia. No hay REPL.
//
// De dónde sale cada función:
//   - spike_compat.h (C): Center_Vehicle, Center_Vehicle_Short, Coast_Motors,
//     Hold_Motors, Reset_Gyro(float grados), Get_Gyro, Forward,
//     Follow_Reference, Advance_For_Degrees (reversa si speed < 0),
//     Turn_For_Degrees(dir, speed, grados, tire_turn),
//     Small_Turn(dir, speed, grados, tire_turn), Close_Serial.
//   - spike_hiwonder.cpp (reto 1): Spike_Hiwonder_Init, Spike_Break_Motors.
//   - spike_reto2.cpp (este reto): lo que la misión usa con otra firma:
//     Turn_For_Degrees y Small_Turn con `await`, Advance_For_distance en mm,
//     y Spike_Reto2_Init.
//
// tire_turn va en las unidades del hub (40 ≈ tope, 15 ≈ un tercio):
// Spike_Reto2_Init ajusta la escala, porque turn() hacía
// run_to_relative_position(port.A, tire_turn*4.29*dir) y el reto 1 definió el
// tope como 179° de ese motor. Ver reto2_config.h.

#include "common_var.h"
#include "spike_compat/spike_compat.h"

// Reto 1: abre la placa, instala la parada de emergencia (Ctrl+C), comprueba
// el firmware y el servo, centra la dirección y calibra el giroscopio 5 s con
// el robot QUIETO. Devuelve 0 si todo fue bien.
int Spike_Hiwonder_Init(void);
// Reto 1: br() del hub -> parada de la propulsión (el servo ya sostiene).
void Spike_Break_Motors(void);

// Spike_Hiwonder_Init + conversiones del reto 2 (tire_turn del hub y grados
// del hub -> ticks por la rueda real). Devuelve 0 si todo fue bien.
int Spike_Reto2_Init(void);

// turn_special()/turns_special() del hub: con await = true la dirección llega
// a su posición antes de mover la propulsión.
void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn, bool await);
void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn, bool await);

// Avanza `distance` milímetros manteniendo el rumbo `reference` (reversa si
// speed < 0). Usa el diámetro real de la rueda (reto2_config.h).
void Spike_Advance_For_distance(int speed, int distance, int reference);

#endif // SPIKE_H
