// Obstacle Challenge (reto 2) on the Hiwonder controller board: the SPIKE API
// functions used by the mission that spike_compat does not provide with the
// same signature (see spike.h). Board start-up is the Open Challenge one
// (Spike_Hiwonder_Init); this file only changes the scale factors that the
// Obstacle Challenge mission uses differently.

#include "spike.h"
#include "reto2_config.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

// Initialises the board through Spike_Hiwonder_Init (blocks during the gyro
// calibration), then rescales the spike_compat configuration for the
// Obstacle Challenge mission and prints the resulting factors.
// Returns 0 on success, -1 if the board init fails.
int Spike_Reto2_Init(void) {
    if (Spike_Hiwonder_Init() != 0) {
        return -1;
    }
    spike_compat::Config cfg = spike_compat::Spike_Get_Config();
    // The Open Challenge passes tire_turn = 179 (full lock, in hub motor
    // degrees); the Obstacle Challenge mission passes turn() units (40 = almost
    // full lock): same scale multiplied by the 4.29 degrees per unit.
    cfg.tire_turn_to_steering *= kGradosMotorPorTireTurn;
    // Hub motor degrees -> mm (LEGO geometry) -> M4 encoder ticks (real wheel).
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

// await = true: the steering reaches its position before the drive motor
// moves (hub turn_special / turns_special). Commands the steering servo and
// sleeps kEsperaDireccionAwaitMs (ms). The spike_compat turn sends the same
// position again afterwards, so repeating it is harmless.
void esperar_direccion(int direction, int tire_turn, bool await) {
    if (!await) return;
    spike_compat::Steer_Tire_Turn(direction, tire_turn);
    std::this_thread::sleep_for(std::chrono::milliseconds(kEsperaDireccionAwaitMs));
}

}  // namespace

// Overload with `await`: optional steering wait, then the 4-argument
// spike_compat turn (blocks until the gyro has turned `degrees`).
void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn, bool await) {
    esperar_direccion(direction, tire_turn, await);
    Spike_Turn_For_Degrees(direction, speed, degrees, tire_turn);
}

// Overload with `await`: optional steering wait, then the 4-argument
// spike_compat small turn.
void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn, bool await) {
    esperar_direccion(direction, tire_turn, await);
    Spike_Small_Turn(direction, speed, degrees, tire_turn);
}

void Spike_Advance_For_distance(int speed, int distance, int reference) {
    // Same as the hub: ag(speed, 1.4 * distance * 360 / 196.035, reference).
    // The sign of speed gives the direction (reverse if negative).
    Spike_Advance_For_Degrees(speed, static_cast<int>(std::lround(distance / kMmPorGradoSpike)),
                              reference);
}
