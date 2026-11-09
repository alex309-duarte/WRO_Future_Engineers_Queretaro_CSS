#ifndef FAKE_BRICK_H
#define FAKE_BRICK_H

// Fake board for the offline tests: defines the hiwonder::Brick methods used
// by spike_compat and spike_hiwonder.cpp, without a serial port. It records
// every command and simulates the yaw: with the steering to the right (+1) the
// yaw decreases, as on the hub (after a right turn the Open Challenge code
// continues with reference -90).

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <string>
#include <vector>

namespace fake {

// Port that the fake board "discovers" when Config::device is empty.
constexpr const char *kDispositivoDescubierto =
    "/dev/serial/by-id/usb-1a86_USB_Single_Serial_FAKE-if00";

// Recorded state of the fake board; tests set the inputs and read the outputs.
struct Estado {
    std::vector<std::string> eventos;  // "open", "servo 1 500 1000", "calibrate 5000"...
    std::string dispositivo_config = "not constructed";  // Config::device received
    std::string dispositivo_parada;    // port opened by the emergency stop
    std::uint8_t motor_mask = 1U << 3; // capabilities: position control on M4
    double yaw_deg = 0.0;              // simulated yaw, deg
    double ultima_direccion = 0.0;     // last steering, normalized -1..1
    hiwonder::SteeringCalibration ultima_calibracion{};
    float ultima_velocidad_rps = 0.0F; // last drive speed, rev/s (0 after stop)
    double ultimo_reset_yaw = 1e9;     // last reset_yaw() value (1e9 = never)
    std::int32_t ultimo_avance_ticks = 0;  // last move_motor_relative() delta
    int paradas = 0;                   // number of stop_all() calls
    int holds = 0;                     // number of hold_motor_position() calls
    std::int16_t posicion_servo = 750;  // what read_bus_servo_position returns (= kDireccionCentro)
    bool servo_responde = true;        // false = servo reply marked invalid
};

// Access to the single fake state; reiniciar() resets it to defaults.
Estado &estado();
void reiniciar();

}  // namespace fake

#endif // FAKE_BRICK_H
