// Design v7 (main branch): startup of the Hiwonder board and the variants of the
// team's SPIKE API that spike_compat does not provide with the same signature
// (see spike.h). Spike_Hiwonder_Init() opens the board, installs the
// emergency stop, checks firmware and steering servo, centres the steering
// and calibrates the gyro; it is called once at the start of each challenge.

#include "spike.h"
#include "hiwonder_config.h"
#include "parada_emergencia.h"

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

// turn() on the hub rotated the steering motor 179° relative, i.e. to full
// lock. With tire_turn_to_steering = 1/179, that same value gives normalized
// steering ±1: the calibrated limits of the HX-35H.
constexpr int kGiroLlantaTope = 179;

std::unique_ptr<hiwonder::Brick> g_brick;   // owned here, lent to spike_compat

// Returns the board; throws if Spike_Hiwonder_Init() has not succeeded.
hiwonder::Brick &placa() {
    if (!g_brick || !spike_compat::Spike_Is_Attached()) {
        throw std::logic_error("call Spike_Hiwonder_Init() before using the API");
    }
    return *g_brick;
}

// The firmware has no command watchdog: if the program ends with the motor
// running, it keeps the last speed. This stops it on exit and on an uncaught
// exception. Signals are handled by parada_emergencia.cpp (emergency stop).
void parar_motores() noexcept {
    try {
        if (g_brick && g_brick->is_open()) g_brick->stop_all();
    } catch (...) {
    }
}

// std::terminate handler: stop the motors, then abort as usual.
[[noreturn]] void parar_y_abortar() noexcept {
    parar_motores();
    std::abort();
}

// WRO_MAX_RPS lowers the speed limit (rev/s) without recompiling. Distances
// (encoder) and turns (yaw) do not change; only the time taken. Unset or
// empty = kRpsVelocidad100; values outside (0, 5.7] throw.
float tope_rps() {
    const char *texto = std::getenv("WRO_MAX_RPS");
    if (texto == nullptr || *texto == '\0') {
        return static_cast<float>(kRpsVelocidad100);
    }
    char *fin = nullptr;
    const double valor = std::strtod(texto, &fin);
    if (fin == texto || *fin != '\0' || !(valor > 0.0) || valor > kRpsVelocidad100) {
        throw std::invalid_argument(std::string("WRO_MAX_RPS=") + texto +
                                    ": must be between 0 and 5.7 r/s");
    }
    return static_cast<float>(valor);
}

}  // namespace

// Startup sequence. Order matters: nothing is moved until the emergency stop
// is installed, the firmware is confirmed and the servo is known to be inside
// its travel. Blocks ~1.2 s for centring plus kSegundosCalibracionGiro for
// the gyro. Returns 0 on success; on any error prints it, stops the motors,
// releases the board and returns -1.
int Spike_Hiwonder_Init(void) {
    try {
        // Translate hiwonder_config.h into the spike_compat configuration.
        spike_compat::Config cfg;
        cfg.rps_at_speed_100 = kRpsVelocidad100;
        cfg.max_rps_limit = tope_rps();
        cfg.pd_output_to_steering = kPdADireccion;
        cfg.tire_turn_to_steering = 1.0 / kGiroLlantaTope;
        cfg.spike_degrees_to_ticks = kGradosSpikeATicks;
        cfg.advance_finish_ticks = kTicksFinAvance;
        // The loops wait for each new IMU sample (20 Hz); the period only
        // bounds that wait. 50 ms, as on the bench, so the servo reaches each
        // setpoint before the next one.
        cfg.control_period = std::chrono::milliseconds(50);
        cfg.heading_deadband_deg = kBandaMuertaRumboGrados;
        cfg.steering.servo_id = kServoDireccion;
        cfg.steering.left = kDireccionIzquierda;
        cfg.steering.center = kDireccionCentro;
        cfg.steering.right = kDireccionDerecha;
        cfg.steering.duration_ms = kDireccionMs;

        // Without Config::device, the Brick discovers the port on open():
        // HIWONDER_DEVICE or the only Hiwonder CH343 in /dev/serial/by-id.
        g_brick = std::make_unique<hiwonder::Brick>();
        g_brick->open();
        std::atexit(parar_motores);
        std::set_terminate(parar_y_abortar);
        // Before moving anything: Ctrl+C (and kill, Ctrl+Z, crashes) leaves
        // the motors in COAST and ends the process, even in the middle of a
        // blocking turn or advance.
        Parada_Emergencia_Instalar(g_brick->device().c_str(), kServoDireccion,
                                   kSoltarDireccionAlParar);

        // The drive relies on firmware position control of M4 (pos-v8).
        const auto caps = g_brick->motor_position_capabilities();
        if (!caps.valid || !(caps.motor_mask & (1U << g_brick->position_motor_id()))) {
            throw std::runtime_error(
                "the firmware has no position control on M4 (is pos-v8 loaded?)");
        }

        // Before sending ANY position to the servo: if it is not already inside
        // the steering travel, centring it would rotate it past the limit and
        // break the linkage (this happened with the centre at 500 and the
        // servo assembled at ~0). Allowed: [right - margin, left + margin],
        // 584..916 with the current hiwonder_config.h.
        const auto actual = g_brick->read_bus_servo_position(
            kServoDireccion, std::chrono::milliseconds(800));
        const int minimo = std::min(kDireccionDerecha, kDireccionIzquierda) - kDireccionMargenArranque;
        const int maximo = std::max(kDireccionDerecha, kDireccionIzquierda) + kDireccionMargenArranque;
        if (!actual.valid || actual.id != kServoDireccion) {
            throw std::runtime_error("the steering servo did not respond; nothing will move");
        }
        if (actual.position < minimo || actual.position > maximo) {
            throw std::runtime_error(
                "the steering servo is at " + std::to_string(actual.position) +
                ", outside the allowed travel " + std::to_string(minimo) + ".." +
                std::to_string(maximo) + ". Centring it would drive it against the end stop. "
                "Check kDireccionCentro in hiwonder_config.h or the horn mounting");
        }
        std::printf("Steering servo at %d (allowed %d..%d)\n",
                    actual.position, minimo, maximo);

        // Centre slowly (1 s) BEFORE calibrating: a moving servo shakes the IMU.
        g_brick->set_bus_servo_position(kServoDireccion, kDireccionCentro, 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        std::printf("Calibrating gyroscope for %d s: do NOT move the robot\n",
                    kSegundosCalibracionGiro);
        g_brick->calibrate_gyro(std::chrono::seconds(kSegundosCalibracionGiro));

        spike_compat::Spike_Attach(g_brick.get(), cfg);
        std::printf("Hiwonder board ready on %s; speed limit %.2f r/s\n",
                    g_brick->device().c_str(), cfg.max_rps_limit);
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Hiwonder board: %s\n", error.what());
        parar_motores();
        spike_compat::Spike_Detach();
        g_brick.reset();
        return -1;
    }
}

// Mission-side variant in decidegrees (as the hub's reset_yaw()).
void Spike_Reset_Gyro(int decidegrees) {
    Spike_Reset_Gyro(static_cast<float>(decidegrees) / 10.0F);
}

void Spike_Print_Gyro(void) {
    // pg() on the hub returned tilt_angles()[0], in decidegrees.
    std::printf("gyro: %ld\n", std::lround(Spike_Get_Gyro() * 10.0F));
}

// Full-lock turn, like turn() on the hub: tire_turn = 179 -> steering ±1.
void Spike_Turn_For_Degrees(int direction, int speed, int degrees) {
    Spike_Turn_For_Degrees(direction, speed, static_cast<float>(degrees), kGiroLlantaTope);
}

void Spike_Center_Vehicle_Short(bool advance) {
    if (advance) {
        // cvca(): drive at 70 % while centring, and leave it running.
        auto &b = placa();
        const auto &cfg = spike_compat::Spike_Get_Config();
        const double rps = std::min(0.7 * cfg.rps_at_speed_100,
                                    static_cast<double>(cfg.max_rps_limit));
        b.set_motor_speed(b.position_motor_id(), static_cast<float>(rps));
    }
    Spike_Center_Vehicle_Short();
}

void Spike_Break_Motors(void) {
    // br() braked both motors. The steering servo already holds its
    // position; the drive is stopped.
    Spike_Coast_Motors();
}

// No-op: there is no REPL any more.
void Spike_Send_Serial_Data(const char *) {}
