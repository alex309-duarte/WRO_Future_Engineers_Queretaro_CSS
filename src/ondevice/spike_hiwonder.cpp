// Rama reto1-hiwonder: arranque de la placa Hiwonder y las variantes de la API
// SPIKE del equipo que spike_compat no tiene con la misma firma (ver spike.h).

#include "spike.h"
#include "hiwonder_config.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

// turn() del hub giraba el motor de dirección 179° relativos, es decir, a
// tope. Con tire_turn_to_steering = 1/179, ese mismo valor da la dirección
// normalizada ±1: los topes calibrados del HX-12H.
constexpr int kGiroLlantaTope = 179;

std::unique_ptr<hiwonder::Brick> g_brick;

hiwonder::Brick &placa() {
    if (!g_brick || !spike_compat::Spike_Is_Attached()) {
        throw std::logic_error("llamar a Spike_Hiwonder_Init() antes de usar la API");
    }
    return *g_brick;
}

// El firmware no tiene watchdog de comandos: si el programa termina con el
// motor andando, sigue a la última velocidad. Esto lo para al salir y ante una
// excepción no capturada. No cubre la muerte por señal (el segundo Ctrl+C,
// que main.cpp deja en SIG_DFL).
void parar_motores() noexcept {
    try {
        if (g_brick && g_brick->is_open()) g_brick->stop_all();
    } catch (...) {
    }
}

[[noreturn]] void parar_y_abortar() noexcept {
    parar_motores();
    std::abort();
}

// WRO_MAX_RPS baja el tope de velocidad sin recompilar. Las distancias
// (encoder) y los giros (yaw) no cambian; sólo el tiempo.
float tope_rps() {
    const char *texto = std::getenv("WRO_MAX_RPS");
    if (texto == nullptr || *texto == '\0') {
        return static_cast<float>(kRpsVelocidad100);
    }
    char *fin = nullptr;
    const double valor = std::strtod(texto, &fin);
    if (fin == texto || *fin != '\0' || !(valor > 0.0) || valor > kRpsVelocidad100) {
        throw std::invalid_argument(std::string("WRO_MAX_RPS=") + texto +
                                    ": debe estar entre 0 y 5.7 r/s");
    }
    return static_cast<float>(valor);
}

}  // namespace

int Spike_Hiwonder_Init(void) {
    try {
        spike_compat::Config cfg;
        cfg.rps_at_speed_100 = kRpsVelocidad100;
        cfg.max_rps_limit = tope_rps();
        cfg.pd_output_to_steering = kPdADireccion;
        cfg.tire_turn_to_steering = 1.0 / kGiroLlantaTope;
        cfg.spike_degrees_to_ticks = kGradosSpikeATicks;
        // Los lazos esperan cada muestra nueva del IMU (20 Hz); el periodo sólo
        // acota esa espera. 50 ms, como en banco, para que el servo alcance
        // cada consigna antes de la siguiente.
        cfg.control_period = std::chrono::milliseconds(50);
        cfg.heading_deadband_deg = kBandaMuertaRumboGrados;
        cfg.steering.servo_id = kServoDireccion;
        cfg.steering.left = kDireccionIzquierda;
        cfg.steering.center = kDireccionCentro;
        cfg.steering.right = kDireccionDerecha;
        cfg.steering.duration_ms = kDireccionMs;

        hiwonder::Config config_placa;
        config_placa.device = HIWONDER_DISPOSITIVO;
        g_brick = std::make_unique<hiwonder::Brick>(config_placa);
        g_brick->open();
        std::atexit(parar_motores);
        std::set_terminate(parar_y_abortar);

        const auto caps = g_brick->motor_position_capabilities();
        if (!caps.valid || !(caps.motor_mask & (1U << g_brick->position_motor_id()))) {
            throw std::runtime_error(
                "el firmware no controla la posición de M4 (¿está cargado pos-v8?)");
        }

        // Centrar despacio ANTES de calibrar: el servo en movimiento sacude la IMU.
        g_brick->set_bus_servo_position(kServoDireccion, kDireccionCentro, 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        std::printf("Calibrando giroscopio %d s: NO mover el robot\n",
                    kSegundosCalibracionGiro);
        g_brick->calibrate_gyro(std::chrono::seconds(kSegundosCalibracionGiro));

        spike_compat::Spike_Attach(g_brick.get(), cfg);
        std::printf("Placa Hiwonder lista; tope de velocidad %.2f r/s\n",
                    cfg.max_rps_limit);
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Placa Hiwonder: %s\n", error.what());
        parar_motores();
        spike_compat::Spike_Detach();
        g_brick.reset();
        return -1;
    }
}

void Spike_Reset_Gyro(int decidegrees) {
    Spike_Reset_Gyro(static_cast<float>(decidegrees) / 10.0F);
}

void Spike_Print_Gyro(void) {
    // pg() del hub devolvía tilt_angles()[0], en decigrados.
    std::printf("gyro: %ld\n", std::lround(Spike_Get_Gyro() * 10.0F));
}

void Spike_Turn_For_Degrees(int direction, int speed, int degrees) {
    Spike_Turn_For_Degrees(direction, speed, static_cast<float>(degrees), kGiroLlantaTope);
}

void Spike_Center_Vehicle_Short(bool advance) {
    if (advance) {
        // cvca(): propulsión al 70 % mientras centra, y la deja andando.
        auto &b = placa();
        const auto &cfg = spike_compat::Spike_Get_Config();
        const double rps = std::min(0.7 * cfg.rps_at_speed_100,
                                    static_cast<double>(cfg.max_rps_limit));
        b.set_motor_speed(b.position_motor_id(), static_cast<float>(rps));
    }
    Spike_Center_Vehicle_Short();
}

void Spike_Break_Motors(void) {
    // br() frenaba los dos motores. El servo de dirección ya sostiene su
    // posición; la propulsión se para.
    Spike_Coast_Motors();
}

void Spike_Send_Serial_Data(const char *) {}
