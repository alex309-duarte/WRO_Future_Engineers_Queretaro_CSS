#ifndef HIWONDER_CONFIG_H
#define HIWONDER_CONFIG_H

// Robot calibration for the Hiwonder controller board (design v7): every value
// that depends on the mechanical assembly lives here (steering servo HX-35H,
// drive motor M4, IMU heading PD). Used by spike_hiwonder.cpp in both the Open
// Challenge and the Obstacle Challenge. Values marked CHECK come from the test
// bench (2026-10-04), not from the assembled robot; HIWONDER.md explains how
// to verify them.

#include <cstdint>

// Board serial port (CH343 USB-UART): discovered automatically at startup, so
// any board works without editing code. With several boards connected, pick
// one explicitly:
// HIWONDER_DEVICE=/dev/serial/by-id/usb-1a86_USB_Single_Serial_<serial>-if00

// --- Steering: HX-35H serial bus servo -------------------------------------
// Positions 0..1000 (~0.24° per unit). On the bench, with "right" at the
// higher position the heading correction came out reversed, so here right is
// the lower position. CHECK: with the wheels lifted, the right position must
// turn the wheels to the right.
//
// STEERING LIMIT: ±35° from centre; beyond the mechanical limit the wheel hits
// the chassis. Every steering command (PD, full-lock turns, centring) goes
// through set_steering, which clamps to ±1 = these positions, so this is the
// hard limit. 35° / 0.24 = 145.8 units; the cast truncates (145 = 34.8°).
constexpr double        kGradosPorUnidadServo = 0.24;   // deg per servo unit
constexpr double        kDireccionMaxGrados   = 35.0;   // deg, max steering angle
constexpr std::uint16_t kDireccionRecorrido   =
    static_cast<std::uint16_t>(kDireccionMaxGrados / kGradosPorUnidadServo);  // 145
constexpr std::uint8_t  kServoDireccion     = 1;        // bus servo ID
// Centre = wheels straight. 180° of the servo (180 / 0.24 = 750): the linkage
// is assembled with the servo at this position so it lines up with the screws.
constexpr std::uint16_t kDireccionCentro    = 750;
constexpr std::uint16_t kDireccionDerecha   = kDireccionCentro - kDireccionRecorrido;  // 605
constexpr std::uint16_t kDireccionIzquierda = kDireccionCentro + kDireccionRecorrido;  // 895
constexpr std::uint16_t kDireccionMs        = 50;       // ms, servo move time per command
static_assert(kDireccionRecorrido * kGradosPorUnidadServo <= kDireccionMaxGrados,
              "el recorrido de la dirección supera el tope de grados");
// The servo only accepts commands 0..1000: the centre must leave ±145 units
// free on both sides. With the centre at 0 one side would be at -145, which
// cannot be commanded. If "straight" falls near 0 or 1000, the servo horn must
// be remounted.
static_assert(kDireccionCentro >= kDireccionRecorrido &&
              kDireccionCentro + kDireccionRecorrido <= 1000,
              "kDireccionCentro no deja ±35° dentro de 0..1000: remontar el horn");
// At startup the servo must already be inside [right, left] plus this margin
// (21 units = ~5°, i.e. 584..916); otherwise Spike_Hiwonder_Init() refuses to
// move it. Centring from outside the travel would drive the wheel into the
// chassis.
constexpr int kDireccionMargenArranque = 21;
// Emergency stop (Ctrl+C): besides leaving the motors in COAST, it turns off
// the servo torque so it does not push the linkage against a stop. The wheels
// are left free. false = the servo holds its last position.
constexpr bool kSoltarDireccionAlParar = true;

// --- Heading: spike_compat PD ----------------------------------------------
// PD output -> normalized steering -1..1. With 0.133 and ±200 units, 1° of
// error moved the servo 26.7 units, the sensitivity of the hub. The factor is
// scaled by the travel to keep that sensitivity with the ±35° limit (0.183
// with ±145): the correction saturates earlier but responds the same to small
// errors. TUNE on the track.
constexpr double kPdADireccion           = 0.133 * 200.0 / kDireccionRecorrido;
constexpr double kBandaMuertaRumboGrados = 0.3;   // deg, heading error dead band

// --- Drive: motor M4, with position control in the firmware ----------------
// Hub speed=100 -> output shaft rev/s. 5.7 is the firmware limit (pos-v8).
// The WRO_MAX_RPS environment variable can lower it for first tests.
constexpr double kRpsVelocidad100 = 5.7;
// Hub motor degrees -> M4 encoder ticks (1320 per output rev, measured).
// The mapping is only 1:1 if wheel and transmission travel the same distance
// per revolution as on the LEGO robot. CALIBRATE: Spike_Advance_For_Degrees(
// 100, 1350, 0) must cover the same distance as with the hub.
constexpr double kGradosSpikeATicks = 1320.0 / 360.0;   // ~3.667 ticks per deg
// Spike_Advance_For_Degrees finishes when 40 ticks remain (~11 hub degrees,
// ~1/33 rev) and leaves the drive in COAST, like ag() on the hub. Waiting for
// the firmware instead (0) makes the final settling within ±15 ticks add up to
// ~1 s of slow motion at the end of each segment. TUNE on the track: the robot
// still rolls a little after release.
constexpr std::uint16_t kTicksFinAvance = 40;

// Gyro bias calibration at startup (s), with the robot standing still. On the
// bench, 2 s left ~0.5° of drift in 20 s; the error falls with the square root
// of the averaging time, so 5 s reduces it ~1.6 times.
constexpr int kSegundosCalibracionGiro = 5;

#endif // HIWONDER_CONFIG_H
