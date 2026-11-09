// Offline tests of the Obstacle Challenge (reto 2) layer against the Open
// Challenge fake Hiwonder board (src/ondevice/tests/fake_brick.cpp): no serial
// port and no actuators. They cover the translations that would still compile
// if they were wrong: the hub tire_turn scale, reverse driving (direction and
// sign of the heading correction), advancing in millimetres through the real
// wheel, and advancing with a stop condition.
// They do NOT use assert: it is compiled out with -DNDEBUG and the suite would
// then pass without checking anything. Exit code 0 = all checks passed.

#include "../spike.h"
#include "../reto2_config.h"
#include "../../../ondevice/hiwonder_config.h"
#include "../../../ondevice/tests/fake_brick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

namespace {

int g_fallos = 0;
int g_checks = 0;

// Counts one check; prints the description and counts a failure if !ok.
void check(bool ok, const char *que) {
    ++g_checks;
    if (!ok) {
        ++g_fallos;
        std::printf("  FALLO: %s\n", que);
    }
}

// True if a and b differ by less than tol.
bool cerca(double a, double b, double tol = 1e-6) { return std::fabs(a - b) < tol; }

// Resets the fake board, clears the speed cap and runs Spike_Reto2_Init.
int arrancar() {
    fake::reiniciar();
    unsetenv("WRO_MAX_RPS");
    return Spike_Reto2_Init();
}

// Number of "cancel" (motion cancelled) events recorded by the fake board.
long cancelaciones() {
    const auto &ev = fake::estado().eventos;
    return std::count(ev.begin(), ev.end(), std::string("cancel"));
}

// Expected encoder ticks per hub degree (1320 ticks per wheel revolution).
double ticks_por_grado_hub() {
    return kMmPorGradoSpike / (M_PI * kDiametroRuedaMm) * 1320.0;
}

void test_init() {
    check(arrancar() == 0, "Spike_Reto2_Init arranca con la placa falsa");
    const auto &cfg = spike_compat::Spike_Get_Config();
    check(cerca(cfg.tire_turn_to_steering * 40.0, 40.0 * 4.29 / 179.0),
          "tire_turn 40 del hub = 0.96 del tope de dirección");
    check(cfg.tire_turn_to_steering * 45.0 > 1.0, "tire_turn 45 pasa del tope (se acota a 1)");
    check(cerca(cfg.spike_degrees_to_ticks, ticks_por_grado_hub()),
          "grados del hub -> ticks por la rueda real");
    check(cerca(cfg.pd_output_to_steering, kPdADireccion) &&
              cfg.steering.center == kDireccionCentro && cfg.steering.right == kDireccionDerecha &&
              cerca(cfg.rps_at_speed_100, kRpsVelocidad100),
          "conserva la calibración del reto 1 (servo, PD, velocidad)");
    check(spike_compat::Spike_Is_Attached(), "deja la capa asociada");
}

void test_init_falla_si_falla_el_reto1() {
    fake::reiniciar();
    unsetenv("WRO_MAX_RPS");
    fake::estado().servo_responde = false;
    check(Spike_Reto2_Init() != 0, "si el arranque del reto 1 falla, el del reto 2 también");
    check(!spike_compat::Spike_Is_Attached(), "y no deja la capa asociada");
}

void test_giro_con_await() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(1, 60, 86, 40, true);   // Corner_Case
    const auto &e = fake::estado();
    check(cerca(e.ultima_direccion, 40.0 * 4.29 / 179.0), "giro a la derecha con tire_turn 40: dirección 0.96");
    check(e.yaw_deg <= -86.0 && e.yaw_deg > -100.0, "gira hasta |yaw| >= 86 (la derecha baja el yaw)");
    check(e.paradas > 0 && e.holds > 0, "al final para y sostiene");
}

void test_giro_sin_await_usa_la_misma_escala() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(-1, 60, 30, 30);        // 4 arguments: goes straight to spike_compat
    check(cerca(fake::estado().ultima_direccion, -30.0 * 4.29 / 179.0),
          "tire_turn 30 a la izquierda = -0.72 también sin await");
}

void test_giro_en_reversa() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(-1, -40, 16, 45, true);  // as in estacionamiento_clockwise (parking)
    const auto &e = fake::estado();
    check(cerca(e.ultima_direccion, -1.0), "tire_turn 45 a la izquierda = tope -1");
    check(std::fabs(e.yaw_deg) >= 16.0, "en reversa también termina cuando |yaw| >= grados");
}

void test_small_turn_vuelve_al_rumbo() {
    arrancar();
    Spike_Reset_Gyro(0);
    fake::estado().yaw_deg = 40.0;                  // after swerving left around a pillar
    Spike_Small_Turn(1, 60, 0, 40, false);
    check(fake::estado().yaw_deg <= 0.0, "small_turn a la derecha sigue hasta que yaw <= 0");
    check(fake::estado().holds > 0, "y sostiene al terminar");
}

void test_avance_adelante() {
    arrancar();
    fake::estado().yaw_deg = 5.0;                   // 5 deg to the left of reference 0
    Spike_Advance_For_Degrees(80, 1000, 0);
    const auto &e = fake::estado();
    check(e.ultimo_avance_ticks == std::lround(1000.0 * ticks_por_grado_hub()),
          "1000 grados del hub -> ticks positivos según la rueda");
    check(e.ultima_direccion > 0.0, "adelante con error positivo corrige a la derecha");
    check(e.paradas > 0, "al terminar deja los motores en coast");
}

void test_avance_en_reversa() {
    arrancar();
    fake::estado().yaw_deg = 5.0;
    Spike_Advance_For_Degrees(80, 1000, 0);
    const double adelante = fake::estado().ultima_direccion;

    arrancar();
    fake::estado().yaw_deg = 5.0;
    Spike_Advance_For_Degrees(-50, 2250, 0);        // parking manoeuvre
    const auto &e = fake::estado();
    check(e.ultimo_avance_ticks == -std::lround(2250.0 * ticks_por_grado_hub()),
          "speed < 0: ticks negativos (reversa) con la magnitud de los grados");
    check(e.ultima_direccion < 0.0, "en reversa el mismo error corrige al revés");
    check(std::fabs(e.ultima_direccion) < std::fabs(adelante),
          "y con menos ganancia que adelante (como ag_u del hub)");
}

void test_avance_en_mm() {
    arrancar();
    Spike_Advance_For_distance(80, 1000, 0);
    const long esperado = std::lround(1000.0 / (M_PI * kDiametroRuedaMm) * 1320.0);
    check(std::labs(fake::estado().ultimo_avance_ticks - esperado) <= 5,
          "1000 mm -> ticks de una vuelta de rueda por su diámetro (±5 ticks de redondeo)");

    arrancar();
    Spike_Advance_For_distance(-50, 300, 90);
    check(fake::estado().ultimo_avance_ticks < 0, "distancia con speed negativa va en reversa");
}

void test_avance_de_cero_no_revienta() {
    arrancar();
    bool lanzo = false;
    try {
        Spike_Advance_For_distance(80, 0, 0);      // the mission's "hypotenuse - 400" can be 0
        Spike_Advance_For_Degrees(80, 0, 0);
    } catch (const std::exception &) {
        lanzo = true;
    }
    check(!lanzo, "avanzar 0 no lanza (el firmware rechaza un delta de 0 ticks)");
    check(fake::estado().ultimo_avance_ticks == 0, "y no manda ningún movimiento");
    check(fake::estado().paradas > 0, "y deja los motores en coast");
}

void test_avance_hasta_condicion() {
    arrancar();
    bool detenido = spike_compat::Advance_For_Degrees_Until(80, 1000, 0, [] { return true; });
    check(detenido && cancelaciones() == 1, "con la condición cumplida cancela el movimiento y avisa");
    check(fake::estado().paradas > 0, "y deja los motores en coast");

    arrancar();
    detenido = spike_compat::Advance_For_Degrees_Until(80, 1000, 0, [] { return false; });
    check(!detenido && cancelaciones() == 0, "sin condición cumplida completa el avance");
}

void test_tope_de_velocidad() {
    fake::reiniciar();
    setenv("WRO_MAX_RPS", "1.5", 1);
    check(Spike_Reto2_Init() == 0 && cerca(spike_compat::Spike_Get_Config().max_rps_limit, 1.5),
          "WRO_MAX_RPS=1.5 también acota el reto 2");
    unsetenv("WRO_MAX_RPS");
}

}  // namespace

int main() {
    std::printf("Pruebas del reto 2 sobre la placa Hiwonder (placa falsa)\n");
    test_init();
    test_init_falla_si_falla_el_reto1();
    test_giro_con_await();
    test_giro_sin_await_usa_la_misma_escala();
    test_giro_en_reversa();
    test_small_turn_vuelve_al_rumbo();
    test_avance_adelante();
    test_avance_en_reversa();
    test_avance_en_mm();
    test_avance_de_cero_no_revienta();
    test_avance_hasta_condicion();
    test_tope_de_velocidad();
    spike_compat::Spike_Detach();
    std::printf("%d comprobaciones, %d fallos\n", g_checks, g_fallos);
    return g_fallos == 0 ? 0 : 1;
}
