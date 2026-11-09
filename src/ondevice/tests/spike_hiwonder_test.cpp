// Offline tests of the Hiwonder board adapter (spike_hiwonder.cpp +
// spike_compat) against a fake board (fake_brick.cpp). They cover the
// hub-to-board translations, which compile the same even when they are
// wrong: gyro units, full-lock steering, speed limit and startup order.
// They do NOT use assert: it disappears with -DNDEBUG and the suite would
// pass without checking anything. Exit code 0 = all checks passed.

#include "../spike.h"
#include "../hiwonder_config.h"
#include "fake_brick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

int g_fallos = 0;   // failed checks
int g_checks = 0;

// Records one check; prints the description if it fails.
void check(bool ok, const char *que) {
    ++g_checks;
    if (!ok) {
        ++g_fallos;
        std::printf("  FAIL: %s\n", que);
    }
}

bool cerca(double a, double b) { return std::fabs(a - b) < 1e-6; }   // "close"

// Index of the first occurrence of `evento` in the event log, or -1.
int posicion(const std::vector<std::string> &eventos, const std::string &evento) {
    const auto it = std::find(eventos.begin(), eventos.end(), evento);
    return it == eventos.end() ? -1 : static_cast<int>(it - eventos.begin());
}

// Resets the fake board, sets (or clears) WRO_MAX_RPS and runs the startup.
int arrancar(const char *tope_rps) {
    fake::reiniciar();
    if (tope_rps) {
        setenv("WRO_MAX_RPS", tope_rps, 1);
    } else {
        unsetenv("WRO_MAX_RPS");
    }
    return Spike_Hiwonder_Init();
}

// Firmware without position control on M4 must abort the startup before
// anything moves.
void test_arranque_rechaza_firmware_sin_posicion() {
    fake::reiniciar();
    unsetenv("WRO_MAX_RPS");
    fake::estado().motor_mask = 0;
    check(Spike_Hiwonder_Init() != 0, "startup fails without position control on M4");
    check(!spike_compat::Spike_Is_Attached(), "and the layer is left detached");
    check(posicion(fake::estado().eventos, "calibrate 5000") < 0,
          "and it neither calibrates nor moves anything");
}

// True if any servo position or centring command was sent.
bool movio_el_servo() {
    for (const auto &e : fake::estado().eventos) {
        if (e.rfind("servo ", 0) == 0 || e == "centrar") return true;
    }
    return false;
}

void test_arranque_rechaza_servo_fuera_de_recorrido() {
    // What broke the steering once: the servo at ~17 and the centre far away
    // (500). Also tests positions just outside the allowed startup range.
    for (const int pos : {17, -49, 0, 1000,
                          kDireccionDerecha - kDireccionMargenArranque - 1,
                          kDireccionIzquierda + kDireccionMargenArranque + 1}) {
        fake::reiniciar();
        unsetenv("WRO_MAX_RPS");
        fake::estado().posicion_servo = static_cast<std::int16_t>(pos);
        const std::string que = "servo at " + std::to_string(pos);
        check(Spike_Hiwonder_Init() != 0, (que + ": startup fails").c_str());
        check(!movio_el_servo(), (que + ": and no position is sent to the servo").c_str());
    }
    fake::reiniciar();
    fake::estado().servo_responde = false;
    check(Spike_Hiwonder_Init() != 0, "servo not responding: startup fails");
    check(!movio_el_servo(), "servo not responding: nothing moves");
}

// Positions exactly on the edge of the allowed range, and the centre, start.
void test_arranque_acepta_servo_en_el_borde() {
    for (const int pos : {kDireccionDerecha - kDireccionMargenArranque,
                          kDireccionIzquierda + kDireccionMargenArranque,
                          static_cast<int>(kDireccionCentro)}) {
        fake::reiniciar();
        unsetenv("WRO_MAX_RPS");
        fake::estado().posicion_servo = static_cast<std::int16_t>(pos);
        check(Spike_Hiwonder_Init() == 0,
              ("servo at " + std::to_string(pos) + ": starts").c_str());
    }
}

// Checks the steering travel on each side against 35 deg / 0.24 deg per unit.
void test_recorrido_de_35_grados() {
    check(kDireccionIzquierda - kDireccionCentro <= 35.0 / 0.24 &&
          kDireccionCentro - kDireccionDerecha <= 35.0 / 0.24,
          "steering stays within ±35° of centre");
}

// Invalid WRO_MAX_RPS values must fail before the board is opened.
void test_arranque_rechaza_tope_invalido() {
    check(arrancar("9") != 0, "WRO_MAX_RPS=9 exceeds the firmware: fails");
    check(arrancar("abc") != 0, "WRO_MAX_RPS=abc: fails");
    check(arrancar("0") != 0, "WRO_MAX_RPS=0: fails");
    check(posicion(fake::estado().eventos, "open") < 0,
          "an invalid limit fails before the board is opened");
}

// Normal startup: checks the order open -> emergency stop -> centre servo
// -> gyro calibration, and the port and speed limit that result.
void test_arranque_ok() {
    check(arrancar(nullptr) == 0, "startup succeeds with the fake board");
    const auto &ev = fake::estado().eventos;
    const int centrar = posicion(ev, "servo 1 " + std::to_string(kDireccionCentro) + " 1000");
    const int calibrar = posicion(ev, "calibrate " + std::to_string(kSegundosCalibracionGiro * 1000));
    const int parada = posicion(ev, std::string("parada_emergencia 1") +
                                        (kSoltarDireccionAlParar ? " soltar" : " sostener"));
    check(posicion(ev, "open") == 0, "opens the board first");
    check(fake::estado().dispositivo_config.empty(),
          "does not fix the port: the Brick discovers it, so any board works");
    check(fake::estado().dispositivo_parada == fake::kDispositivoDescubierto,
          "the emergency stop opens the same port as the board");
    check(parada == 1, "installs the emergency stop right after opening");
    check(centrar > parada, "and before moving the servo");
    check(centrar > 0, "centres the steering slowly (1 s)");
    check(calibrar > centrar, "calibrates the gyroscope AFTER centring");
    check(spike_compat::Spike_Is_Attached(), "leaves the layer attached");
    check(cerca(spike_compat::Spike_Get_Config().max_rps_limit, kRpsVelocidad100),
          "without WRO_MAX_RPS the limit is the firmware's");
}

// Spike_Reset_Gyro(int) takes decidegrees.
void test_reset_gyro_en_decigrados() {
    arrancar(nullptr);
    Spike_Reset_Gyro(30);  // Oradar_S2L_Wall_Slope() returns x10
    check(cerca(fake::estado().ultimo_reset_yaw, 3.0), "Spike_Reset_Gyro(30) -> yaw 3.0°");
    Spike_Reset_Gyro(-457);
    check(cerca(fake::estado().ultimo_reset_yaw, -45.7), "Spike_Reset_Gyro(-457) -> -45.7°");
    Spike_Reset_Gyro(0);
    check(cerca(fake::estado().ultimo_reset_yaw, 0.0), "Spike_Reset_Gyro(0) -> 0°");
}

// Full-lock 70 deg turn in direction `sentido`: steering goes to the
// configured limit, the turn stops just past 70 deg with the hub's yaw sign,
// and the drive ends stopped and in HOLD.
void test_giro_a_tope(int sentido, std::uint16_t posicion_esperada, const std::string &nombre) {
    arrancar(nullptr);
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(sentido, 100, 70);
    const auto &e = fake::estado();
    // Brick::set_steering maps +1 to calibration.right and -1 to calibration.left.
    const auto &cal = e.ultima_calibracion;
    const std::uint16_t destino = e.ultima_direccion > 0 ? cal.right : cal.left;
    check(cerca(e.ultima_direccion, sentido), (nombre + ": steering at full lock").c_str());
    check(destino == posicion_esperada, (nombre + ": the servo goes to the configured position").c_str());
    check(std::fabs(e.yaw_deg) >= 70.0 && std::fabs(e.yaw_deg) < 85.0,
          (nombre + ": turns until |yaw| >= 70°").c_str());
    check(e.yaw_deg * sentido < 0.0, (nombre + ": the yaw changes in the hub's direction").c_str());
    check(e.paradas > 0 && e.holds > 0, (nombre + ": stops and holds at the end").c_str());
}

// cvca() drives at 70 % of speed 100, but never above the WRO_MAX_RPS limit.
void test_tope_de_velocidad() {
    arrancar(nullptr);
    Spike_Center_Vehicle_Short(true);
    check(cerca(fake::estado().ultima_velocidad_rps, 0.7 * kRpsVelocidad100),
          "cvca without limit: 70 % of speed 100");

    arrancar("1.5");
    check(cerca(spike_compat::Spike_Get_Config().max_rps_limit, 1.5), "WRO_MAX_RPS=1.5 sets the limit");
    Spike_Center_Vehicle_Short(true);
    check(cerca(fake::estado().ultima_velocidad_rps, 1.5), "cvca with limit 1.5 stays at 1.5 r/s");
    check(posicion(fake::estado().eventos, "centrar") >= 0, "cvca centres the steering");
}

// Hub motor degrees are converted to encoder ticks (1320 per revolution).
void test_avance_en_ticks() {
    arrancar(nullptr);
    Spike_Advance_For_Degrees(100, 1350, -90);
    check(fake::estado().ultimo_avance_ticks == 4950,
          "1350° of the hub motor -> 4950 ticks (1320 per revolution)");
}

}  // namespace

int main() {
    std::printf("Hiwonder adapter tests (fake board)\n");
    test_arranque_rechaza_firmware_sin_posicion();
    test_arranque_rechaza_tope_invalido();
    test_arranque_rechaza_servo_fuera_de_recorrido();
    test_arranque_acepta_servo_en_el_borde();
    test_recorrido_de_35_grados();
    test_arranque_ok();
    test_reset_gyro_en_decigrados();
    test_giro_a_tope(1, kDireccionDerecha, "right");
    test_giro_a_tope(-1, kDireccionIzquierda, "left");
    test_tope_de_velocidad();
    test_avance_en_ticks();
    spike_compat::Spike_Detach();
    std::printf("%d checks, %d failures\n", g_checks, g_fallos);
    return g_fallos == 0 ? 0 : 1;
}
