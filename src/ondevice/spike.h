#ifndef SPIKE_H
#define SPIKE_H

// Branch reto1-hiwonder: the team's SPIKE API, backed by the Hiwonder board
// (STM32 with firmware pos-v8) through spike_compat. The hub REPL driver
// (spike.cpp) is no longer compiled; see HIWONDER.md. Included by the mission
// code of the Open Challenge and the Obstacle Challenge.
//
// Functions with the same signature as on the hub come directly from
// spike_compat.h. Declared here are the ones the challenge code uses with a
// different signature or different units; spike_hiwonder.cpp translates them:
//   - Spike_Reset_Gyro(int): DECIDEGREES, like reset_yaw() on the hub. The
//     layer works in degrees, and Oradar_S2L_Wall_Slope() already returns the
//     angle x10.
//   - Spike_Turn_For_Degrees(dir, speed, degrees): full-lock steering, like
//     turn() on the hub (run_to_relative_position(port.A, 179*dir)).
//   - Spike_Center_Vehicle_Short(bool), Spike_Break_Motors(),
//     Spike_Print_Gyro() and Spike_Send_Serial_Data(const char*).

#include "common_var.h"
#include "spike_compat/spike_compat.h"

// Replaces Rasp_Gpio_Power_On_Spike + Spike_Serial_Init + Spike_Interpreter
// + Spike_Initialize_Libraries: opens the board, checks the firmware, centres
// the steering and calibrates the gyro (robot STILL). Returns 0 on success.
int Spike_Hiwonder_Init(void);

// Sets the yaw, given in decidegrees (e.g. -457 = -45.7 deg).
void Spike_Reset_Gyro(int decidegrees);
// Prints the current yaw in decidegrees ("gyro: N").
void Spike_Print_Gyro(void);
// Blocking full-lock turn in `direction` (+1 / -1) at `speed` (0..100) until
// |yaw| reaches `degrees`.
void Spike_Turn_For_Degrees(int direction, int speed, int degrees);
// Centres the steering; with advance = true the drive also runs at 70 %
// and is left running.
void Spike_Center_Vehicle_Short(bool advance);
// Stops all motors in COAST (the steering servo keeps its position).
void Spike_Break_Motors(void);
void Spike_Send_Serial_Data(const char* data);   // no-op: there is no REPL any more

#endif // SPIKE_H
