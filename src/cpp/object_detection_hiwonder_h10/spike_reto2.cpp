// Reto 2 sobre la placa Hiwonder: las funciones de la API SPIKE de la misión
// que spike_compat no tiene con la misma firma (ver spike.h). El arranque de
// la placa es el del reto 1 (Spike_Hiwonder_Init); aquí sólo se cambian las
// escalas que la misión del reto 2 usa de otra forma.

#include "spike.h"
#include "reto2_config.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

int Spike_Reto2_Init(void) {
    if (Spike_Hiwonder_Init() != 0) {
        return -1;
    }
    spike_compat::Config cfg = spike_compat::Spike_Get_Config();
    // El reto 1 pasa tire_turn = 179 (el tope, en grados del motor del hub);
    // la misión del reto 2 pasa las unidades de turn() (40 ≈ tope): misma
    // escala multiplicada por los 4.29 grados por unidad.
    cfg.tire_turn_to_steering *= kGradosMotorPorTireTurn;
    // Grados del motor del hub -> mm (geometría LEGO) -> ticks de M4 (rueda real).
    cfg.spike_degrees_to_ticks =
        kMmPorGradoSpike / (M_PI * kDiametroRuedaMm) *
        static_cast<double>(hiwonder::EncoderCalibration{}.ticks_per_output_revolution);
    spike_compat::Spike_Update_Config(cfg);
    std::printf("Reto 2: tire_turn 40 = %.2f del tope de dirección; %.3f ticks por grado del hub "
                "(rueda de %.1f mm; 1000 mm = %ld ticks)\n",
                40.0 * cfg.tire_turn_to_steering, cfg.spike_degrees_to_ticks, kDiametroRuedaMm,
                std::lround(1000.0 / (M_PI * kDiametroRuedaMm) *
                            hiwonder::EncoderCalibration{}.ticks_per_output_revolution));
    return 0;
}

namespace {

// await = true: la dirección llega a su posición antes de mover la
// propulsión (turn_special / turns_special). El giro de spike_compat vuelve a
// mandar la misma posición, así que es inofensivo repetirla.
void esperar_direccion(int direction, int tire_turn, bool await) {
    if (!await) return;
    spike_compat::Steer_Tire_Turn(direction, tire_turn);
    std::this_thread::sleep_for(std::chrono::milliseconds(kEsperaDireccionAwaitMs));
}

}  // namespace

void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn, bool await) {
    esperar_direccion(direction, tire_turn, await);
    Spike_Turn_For_Degrees(direction, speed, degrees, tire_turn);
}

void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn, bool await) {
    esperar_direccion(direction, tire_turn, await);
    Spike_Small_Turn(direction, speed, degrees, tire_turn);
}

void Spike_Advance_For_distance(int speed, int distance, int reference) {
    // Igual que el hub: ag(speed, 1.4 * distance * 360 / 196.035, reference).
    // El signo de speed da el sentido (reversa si es negativo).
    Spike_Advance_For_Degrees(speed, static_cast<int>(std::lround(distance / kMmPorGradoSpike)),
                              reference);
}
