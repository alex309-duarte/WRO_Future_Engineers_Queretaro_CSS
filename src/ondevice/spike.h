#ifndef SPIKE_H
#define SPIKE_H

// Rama reto1-hiwonder: la API SPIKE del equipo, respaldada por la placa
// Hiwonder (STM32 con firmware pos-v8) a través de spike_compat. El driver del
// REPL del hub (spike.cpp) ya no se compila; ver HIWONDER.md.
//
// Las funciones con la misma firma que en el hub vienen directamente de
// spike_compat.h. Aquí se declaran las que el código del reto usa con otra
// firma o en otras unidades; spike_hiwonder.cpp las traduce:
//   - Spike_Reset_Gyro(int): DECIGRADOS, como reset_yaw() del hub. La capa
//     trabaja en grados, y Oradar_S2L_Wall_Slope() ya devuelve el ángulo x10.
//   - Spike_Turn_For_Degrees(dir, speed, degrees): dirección a tope, como
//     turn() del hub (run_to_relative_position(port.A, 179*dir)).
//   - Spike_Center_Vehicle_Short(bool), Spike_Break_Motors(),
//     Spike_Print_Gyro() y Spike_Send_Serial_Data(const char*).

#include "common_var.h"
#include "spike_compat/spike_compat.h"

// Sustituye a Rasp_Gpio_Power_On_Spike + Spike_Serial_Init + Spike_Interpreter
// + Spike_Initialize_Libraries: abre la placa, comprueba el firmware, centra la
// dirección y calibra el giroscopio (robot QUIETO). Devuelve 0 si todo fue bien.
int Spike_Hiwonder_Init(void);

void Spike_Reset_Gyro(int decidegrees);
void Spike_Print_Gyro(void);
void Spike_Turn_For_Degrees(int direction, int speed, int degrees);
void Spike_Center_Vehicle_Short(bool advance);
void Spike_Break_Motors(void);
void Spike_Send_Serial_Data(const char* data);   // no-op: ya no hay REPL

#endif // SPIKE_H
