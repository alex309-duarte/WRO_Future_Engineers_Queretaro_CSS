#ifndef PARADA_EMERGENCIA_H
#define PARADA_EMERGENCIA_H

// Emergency stop on signals (Ctrl+C, kill, terminal closed, Ctrl+Z and
// crashes), used by the Open and Obstacle Challenge programs. The Hiwonder
// board firmware has no command watchdog: if the program dies while the motor
// is running, the motor keeps the last speed. On a signal the handler sends
// the board a stop for all four motors (COAST: the firmware sets both H-bridge
// inputs to 0) and, if requested, turns off the torque of the steering servo.
// It then ends the process without returning to the program, so no
// half-finished loop can send another speed after the stop.

#include <cstddef>
#include <cstdint>

// Opens a second file descriptor on the board's serial port (hiwonder::Brick
// has already configured it at 1 Mbaud) and pre-builds the stop frames.
// dispositivo: serial device path; servo_direccion: bus ID of the steering
// servo; soltar_direccion: also turn off the steering servo torque.
// Call after Brick::open(). Throws std::runtime_error if the port cannot be
// opened.
void Parada_Emergencia_Instalar(const char *dispositivo, std::uint8_t servo_direccion,
                                bool soltar_direccion);

// Registers another port that must also be stopped (the lidar: if the
// process ends without stopping it, its motor keeps spinning). Opens a second
// descriptor on "dispositivo" (already configured by its driver) and copies
// "tramas" (largo bytes), which the handler writes after the board frames.
// Installs the handler if it is not installed yet, so it also covers the
// start-up phase before the board is opened.
// Throws std::runtime_error if the port cannot be opened.
void Parada_Emergencia_Agregar_Puerto(const char *dispositivo, const unsigned char *tramas,
                                      std::size_t largo);

#endif // PARADA_EMERGENCIA_H
