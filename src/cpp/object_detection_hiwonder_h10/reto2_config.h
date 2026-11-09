#ifndef RETO2_CONFIG_H
#define RETO2_CONFIG_H

// Obstacle Challenge (reto 2): unit conversions that let the mission code,
// originally written for the LEGO SPIKE hub (design v6), run on the Hiwonder
// controller board (design v7). The robot calibration itself (steering servo,
// heading PD, speed, emergency stop) is shared with the Open Challenge and
// lives in ../../ondevice/hiwonder_config.h, so a change there applies to both
// challenges. This file holds only what the Open Challenge does not need.

// Diameter, in mm, of the drive wheel moved by motor port M4. This is the only
// value that has to be measured: with it, Spike_Advance_For_distance(..., mm, ...)
// travels that many millimetres, and the "hub degrees" passed to
// Spike_Advance_For_Degrees (e.g. 2250 in the parking manoeuvre) are converted
// to the same distance the LEGO robot used to travel.
// CHECK: Spike_Advance_For_distance(50, 1000, 0) must travel 1000 mm; if it
// travels L mm, multiply this value by L / 1000.
constexpr double kDiametroRuedaMm = 65.0;

// SPIKE hub drive geometry, taken from spike.cpp:
// ag(speed, 1.4 * mm * 360 / 196.035). A LEGO wheel with a 196.035 mm
// circumference behind a 1.4 gear ratio gives the millimetres travelled per
// degree of the hub's drive motor (mm/deg).
constexpr double kMmPorGradoSpike = 196.035 / (1.4 * 360.0);

// Hub turn()/small_turn() steered with run_to_relative_position(port.A,
// tire_turn*4.29*dir), i.e. 4.29 steering-motor degrees per tire_turn unit.
// The Open Challenge maps the full steering lock of the HX-35H servo
// (+/-35 deg, kDireccionMaxGrados in hiwonder_config.h) to 179 deg of that
// motor (kGiroLlantaTope in spike_hiwonder.cpp), so tire_turn 40 = 0.96 of
// full lock, 45 = full lock, 30 = 0.72, 15 = 0.36.
constexpr double kGradosMotorPorTireTurn = 4.29;

// Hub turn_special()/turns_special() (await = true) waited for the steering
// to reach its position before driving. Time, in ms, the HX-35H servo is
// given to reach the commanded steering position (it needs roughly 100 ms).
constexpr int kEsperaDireccionAwaitMs = 150;

#endif // RETO2_CONFIG_H
