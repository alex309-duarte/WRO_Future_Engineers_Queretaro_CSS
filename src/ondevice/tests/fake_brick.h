#ifndef FAKE_BRICK_H
#define FAKE_BRICK_H

// Placa falsa para las pruebas offline: define los métodos de hiwonder::Brick
// que usan spike_compat y spike_hiwonder.cpp, sin puerto serie. Registra cada
// orden y simula el yaw: con la dirección a la derecha (+1) el yaw baja, como
// en el hub (tras un giro a la derecha el reto sigue con referencia -90).

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <string>
#include <vector>

namespace fake {

struct Estado {
    std::vector<std::string> eventos;  // "open", "servo 1 500 1000", "calibrate 5000"...
    std::uint8_t motor_mask = 1U << 3; // capacidades: posición en M4
    double yaw_deg = 0.0;
    double ultima_direccion = 0.0;     // normalizada -1..1
    hiwonder::SteeringCalibration ultima_calibracion{};
    float ultima_velocidad_rps = 0.0F;
    double ultimo_reset_yaw = 1e9;
    std::int32_t ultimo_avance_ticks = 0;
    int paradas = 0;
    int holds = 0;
};

Estado &estado();
void reiniciar();

}  // namespace fake

#endif // FAKE_BRICK_H
