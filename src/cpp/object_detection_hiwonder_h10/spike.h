#ifndef SPIKE_H
#define SPIKE_H

// Obstacle Challenge (reto 2) on the Hiwonder controller board: the same
// SPIKE-style API that object_detection_original_h10 used with the LEGO SPIKE
// hub (design v6), backed by the layer already proven on the track by the
// Open Challenge (src/ondevice): spike_compat + spike_hiwonder.cpp +
// hiwonder_config.h + parada_emergencia (emergency stop). There is no REPL.
//
// Where each function comes from:
//   - spike_compat.h (C API): Center_Vehicle, Center_Vehicle_Short,
//     Coast_Motors, Hold_Motors, Reset_Gyro(float degrees), Get_Gyro, Forward,
//     Follow_Reference, Advance_For_Degrees (reverse if speed < 0),
//     Turn_For_Degrees(dir, speed, degrees, tire_turn),
//     Small_Turn(dir, speed, degrees, tire_turn), Close_Serial.
//   - spike_hiwonder.cpp (Open Challenge): Spike_Hiwonder_Init,
//     Spike_Break_Motors.
//   - spike_reto2.cpp (this challenge): the calls the mission uses with a
//     different signature: Turn_For_Degrees and Small_Turn with `await`,
//     Advance_For_distance in mm, and Spike_Reto2_Init.
//
// tire_turn is given in hub units (40 = almost full lock, 15 = about one
// third): Spike_Reto2_Init rescales it, because the hub's turn() called
// run_to_relative_position(port.A, tire_turn*4.29*dir) and the Open Challenge
// defines full lock as 179 deg of that motor. See reto2_config.h.

#include "common_var.h"
#include "spike_compat/spike_compat.h"

// From the Open Challenge: opens the Hiwonder board, installs the emergency
// stop (Ctrl+C), checks the firmware and the steering servo, centres the
// steering and calibrates the gyro for 5 s with the robot STANDING STILL.
// Returns 0 on success.
int Spike_Hiwonder_Init(void);
// From the Open Challenge: equivalent of the hub's br(); stops the drive
// motor (the steering servo already holds its position).
void Spike_Break_Motors(void);

// Spike_Hiwonder_Init plus the Obstacle Challenge conversions (hub tire_turn
// units, and hub degrees -> encoder ticks for the real wheel). Blocks for the
// gyro calibration. Returns 0 on success, -1 if the board init fails.
int Spike_Reto2_Init(void);

// Hub turn_special()/turns_special(): turn until the gyro has rotated
// `degrees` (deg) at `speed` (%), with steering `tire_turn` (hub units) to
// side `direction` (1 = right, -1 = left). With await = true the steering
// reaches its position before the drive motor starts. Blocks until done.
void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn, bool await);
void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn, bool await);

// Drives `distance` millimetres while holding heading `reference` (deg),
// at `speed` (%; reverse if speed < 0). Uses the real wheel diameter
// (kDiametroRuedaMm in reto2_config.h). Blocks until the distance is covered.
void Spike_Advance_For_distance(int speed, int distance, int reference);

#endif // SPIKE_H
