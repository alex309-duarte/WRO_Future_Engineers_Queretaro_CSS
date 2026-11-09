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
        std::printf("  FAIL: %s\n", que);
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
    check(arrancar() == 0, "Spike_Reto2_Init starts with the fake board");
    const auto &cfg = spike_compat::Spike_Get_Config();
    check(cerca(cfg.tire_turn_to_steering * 40.0, 40.0 * 4.29 / 179.0),
          "hub tire_turn 40 = 0.96 of full steering lock");
    check(cfg.tire_turn_to_steering * 45.0 > 1.0, "tire_turn 45 exceeds full lock (clamped to 1)");
    check(cerca(cfg.spike_degrees_to_ticks, ticks_por_grado_hub()),
          "hub degrees -> ticks for the real wheel");
    check(cerca(cfg.pd_output_to_steering, kPdADireccion) &&
              cfg.steering.center == kDireccionCentro && cfg.steering.right == kDireccionDerecha &&
              cerca(cfg.rps_at_speed_100, kRpsVelocidad100),
          "keeps the Open Challenge calibration (servo, PD, speed)");
    check(spike_compat::Spike_Is_Attached(), "leaves the layer attached");
}

void test_init_falla_si_falla_el_reto1() {
    fake::reiniciar();
    unsetenv("WRO_MAX_RPS");
    fake::estado().servo_responde = false;
    check(Spike_Reto2_Init() != 0, "if the Open Challenge startup fails, the Obstacle Challenge one fails too");
    check(!spike_compat::Spike_Is_Attached(), "and the layer is left detached");
}

void test_giro_con_await() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(1, 60, 86, 40, true);   // Corner_Case
    const auto &e = fake::estado();
    check(cerca(e.ultima_direccion, 40.0 * 4.29 / 179.0), "right turn with tire_turn 40: steering 0.96");
    check(e.yaw_deg <= -86.0 && e.yaw_deg > -100.0, "turns until |yaw| >= 86 (right decreases the yaw)");
    check(e.paradas > 0 && e.holds > 0, "stops and holds at the end");
}

void test_giro_sin_await_usa_la_misma_escala() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(-1, 60, 30, 30);        // 4 arguments: goes straight to spike_compat
    check(cerca(fake::estado().ultima_direccion, -30.0 * 4.29 / 179.0),
          "tire_turn 30 to the left = -0.72, also without await");
}

void test_giro_en_reversa() {
    arrancar();
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(-1, -40, 16, 45, true);  // as in estacionamiento_clockwise (parking)
    const auto &e = fake::estado();
    check(cerca(e.ultima_direccion, -1.0), "tire_turn 45 to the left = full lock -1");
    check(std::fabs(e.yaw_deg) >= 16.0, "in reverse it also ends when |yaw| >= degrees");
}

void test_small_turn_vuelve_al_rumbo() {
    arrancar();
    Spike_Reset_Gyro(0);
    fake::estado().yaw_deg = 40.0;                  // after swerving left around a pillar
    Spike_Small_Turn(1, 60, 0, 40, false);
    check(fake::estado().yaw_deg <= 0.0, "small_turn to the right continues until yaw <= 0");
    check(fake::estado().holds > 0, "and holds at the end");
}

void test_avance_adelante() {
    arrancar();
    fake::estado().yaw_deg = 5.0;                   // 5 deg to the left of reference 0
    Spike_Advance_For_Degrees(80, 1000, 0);
    const auto &e = fake::estado();
    check(e.ultimo_avance_ticks == std::lround(1000.0 * ticks_por_grado_hub()),
          "1000 hub degrees -> positive ticks for the wheel");
    check(e.ultima_direccion > 0.0, "forward with a positive error corrects to the right");
    check(e.paradas > 0, "leaves the motors in coast at the end");
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
          "speed < 0: negative ticks (reverse) with the magnitude of the degrees");
    check(e.ultima_direccion < 0.0, "in reverse the same error corrects the opposite way");
    check(std::fabs(e.ultima_direccion) < std::fabs(adelante),
          "and with less gain than forward (like the hub's ag_u)");
}

void test_avance_en_mm() {
    arrancar();
    Spike_Advance_For_distance(80, 1000, 0);
    const long esperado = std::lround(1000.0 / (M_PI * kDiametroRuedaMm) * 1320.0);
    check(std::labs(fake::estado().ultimo_avance_ticks - esperado) <= 5,
          "1000 mm -> ticks from the wheel circumference (±5 ticks of rounding)");

    arrancar();
    Spike_Advance_For_distance(-50, 300, 90);
    check(fake::estado().ultimo_avance_ticks < 0, "distance with negative speed drives in reverse");
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
    check(!lanzo, "advancing 0 does not throw (the firmware rejects a 0-tick delta)");
    check(fake::estado().ultimo_avance_ticks == 0, "and sends no motion");
    check(fake::estado().paradas > 0, "and leaves the motors in coast");
}

void test_avance_hasta_condicion() {
    arrancar();
    bool detenido = spike_compat::Advance_For_Degrees_Until(80, 1000, 0, [] { return true; });
    check(detenido && cancelaciones() == 1, "with the condition met it cancels the motion and reports it");
    check(fake::estado().paradas > 0, "and leaves the motors in coast");

    arrancar();
    detenido = spike_compat::Advance_For_Degrees_Until(80, 1000, 0, [] { return false; });
    check(!detenido && cancelaciones() == 0, "without the condition met it completes the advance");
}

void test_tope_de_velocidad() {
    fake::reiniciar();
    setenv("WRO_MAX_RPS", "1.5", 1);
    check(Spike_Reto2_Init() == 0 && cerca(spike_compat::Spike_Get_Config().max_rps_limit, 1.5),
          "WRO_MAX_RPS=1.5 also limits the Obstacle Challenge");
    unsetenv("WRO_MAX_RPS");
}

}  // namespace

int main() {
    std::printf("Obstacle Challenge tests on the Hiwonder board (fake board)\n");
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
    std::printf("%d checks, %d failures\n", g_checks, g_fallos);
    return g_fallos == 0 ? 0 : 1;
}
