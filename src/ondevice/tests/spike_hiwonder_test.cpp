// Pruebas offline del adaptador de la rama reto1-hiwonder contra una placa
// falsa (fake_brick.cpp). Cubren las traducciones del hub a la placa, que
// compilan igual aunque estén mal: unidades del giroscopio, dirección a tope,
// tope de velocidad y orden del arranque.
// NO usan assert: desaparece con -DNDEBUG y la suite pasaría sin verificar.

#include "../spike.h"
#include "../hiwonder_config.h"
#include "fake_brick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

int g_fallos = 0;
int g_checks = 0;

void check(bool ok, const char *que) {
    ++g_checks;
    if (!ok) {
        ++g_fallos;
        std::printf("  FALLO: %s\n", que);
    }
}

bool cerca(double a, double b) { return std::fabs(a - b) < 1e-6; }

int posicion(const std::vector<std::string> &eventos, const std::string &evento) {
    const auto it = std::find(eventos.begin(), eventos.end(), evento);
    return it == eventos.end() ? -1 : static_cast<int>(it - eventos.begin());
}

int arrancar(const char *tope_rps) {
    fake::reiniciar();
    if (tope_rps) {
        setenv("WRO_MAX_RPS", tope_rps, 1);
    } else {
        unsetenv("WRO_MAX_RPS");
    }
    return Spike_Hiwonder_Init();
}

void test_arranque_rechaza_firmware_sin_posicion() {
    fake::reiniciar();
    unsetenv("WRO_MAX_RPS");
    fake::estado().motor_mask = 0;
    check(Spike_Hiwonder_Init() != 0, "sin posición en M4 el arranque falla");
    check(!spike_compat::Spike_Is_Attached(), "y la capa queda sin asociar");
    check(posicion(fake::estado().eventos, "calibrate 5000") < 0,
          "y no llega a calibrar ni a mover nada");
}

void test_arranque_rechaza_tope_invalido() {
    check(arrancar("9") != 0, "WRO_MAX_RPS=9 supera el firmware: falla");
    check(arrancar("abc") != 0, "WRO_MAX_RPS=abc: falla");
    check(arrancar("0") != 0, "WRO_MAX_RPS=0: falla");
    check(posicion(fake::estado().eventos, "open") < 0,
          "un tope inválido falla antes de abrir la placa");
}

void test_arranque_ok() {
    check(arrancar(nullptr) == 0, "arranque correcto con la placa falsa");
    const auto &ev = fake::estado().eventos;
    const int centrar = posicion(ev, "servo 1 500 1000");
    const int calibrar = posicion(ev, "calibrate " + std::to_string(kSegundosCalibracionGiro * 1000));
    check(posicion(ev, "open") == 0, "abre la placa primero");
    check(centrar > 0, "centra la dirección despacio (1 s)");
    check(calibrar > centrar, "calibra el giroscopio DESPUÉS de centrar");
    check(spike_compat::Spike_Is_Attached(), "deja la capa asociada");
    check(cerca(spike_compat::Spike_Get_Config().max_rps_limit, kRpsVelocidad100),
          "sin WRO_MAX_RPS el tope es el del firmware");
}

void test_reset_gyro_en_decigrados() {
    arrancar(nullptr);
    Spike_Reset_Gyro(30);  // Oradar_S2L_Wall_Slope() devuelve x10
    check(cerca(fake::estado().ultimo_reset_yaw, 3.0), "Spike_Reset_Gyro(30) -> yaw 3.0°");
    Spike_Reset_Gyro(-457);
    check(cerca(fake::estado().ultimo_reset_yaw, -45.7), "Spike_Reset_Gyro(-457) -> -45.7°");
    Spike_Reset_Gyro(0);
    check(cerca(fake::estado().ultimo_reset_yaw, 0.0), "Spike_Reset_Gyro(0) -> 0°");
}

void test_giro_a_tope(int sentido, std::uint16_t posicion_esperada, const std::string &nombre) {
    arrancar(nullptr);
    Spike_Reset_Gyro(0);
    Spike_Turn_For_Degrees(sentido, 100, 70);
    const auto &e = fake::estado();
    // Brick::set_steering lleva +1 a calibration.right y -1 a calibration.left.
    const auto &cal = e.ultima_calibracion;
    const std::uint16_t destino = e.ultima_direccion > 0 ? cal.right : cal.left;
    check(cerca(e.ultima_direccion, sentido), (nombre + ": dirección a tope").c_str());
    check(destino == posicion_esperada, (nombre + ": el servo va a la posición configurada").c_str());
    check(std::fabs(e.yaw_deg) >= 70.0 && std::fabs(e.yaw_deg) < 85.0,
          (nombre + ": gira hasta |yaw| >= 70°").c_str());
    check(e.yaw_deg * sentido < 0.0, (nombre + ": el yaw va en el sentido del hub").c_str());
    check(e.paradas > 0 && e.holds > 0, (nombre + ": al final para y sostiene").c_str());
}

void test_tope_de_velocidad() {
    arrancar(nullptr);
    Spike_Center_Vehicle_Short(true);
    check(cerca(fake::estado().ultima_velocidad_rps, 0.7 * kRpsVelocidad100),
          "cvca sin tope: 70 % de speed 100");

    arrancar("1.5");
    check(cerca(spike_compat::Spike_Get_Config().max_rps_limit, 1.5), "WRO_MAX_RPS=1.5 fija el tope");
    Spike_Center_Vehicle_Short(true);
    check(cerca(fake::estado().ultima_velocidad_rps, 1.5), "cvca con tope 1.5 queda en 1.5 r/s");
    check(posicion(fake::estado().eventos, "centrar") >= 0, "cvca centra la dirección");
}

void test_avance_en_ticks() {
    arrancar(nullptr);
    Spike_Advance_For_Degrees(100, 1350, -90);
    check(fake::estado().ultimo_avance_ticks == 4950,
          "1350° del motor del hub -> 4950 ticks (1320 por vuelta)");
}

}  // namespace

int main() {
    std::printf("Pruebas del adaptador Hiwonder (placa falsa)\n");
    test_arranque_rechaza_firmware_sin_posicion();
    test_arranque_rechaza_tope_invalido();
    test_arranque_ok();
    test_reset_gyro_en_decigrados();
    test_giro_a_tope(1, kDireccionDerecha, "derecha");
    test_giro_a_tope(-1, kDireccionIzquierda, "izquierda");
    test_tope_de_velocidad();
    test_avance_en_ticks();
    spike_compat::Spike_Detach();
    std::printf("%d comprobaciones, %d fallos\n", g_checks, g_fallos);
    return g_fallos == 0 ? 0 : 1;
}
