#include "spike_compat/spike_compat.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace spike_compat {
namespace detail {

hiwonder::Brick *g_brick = nullptr;
Config g_config{};
bool g_attached = false;

/* Estado del PD, equivalente al "global error" del script del hub. */
double g_prev_error = 0.0;

/* Ganancias tomadas literalmente de spike.cpp (funciones da / apd / ag). */
constexpr double kForwardKp = 0.11;
constexpr double kForwardKd = 0.7;
constexpr double kFollowKp  = 0.08;
constexpr double kFollowKd  = 0.7;
constexpr double kAdvanceKp = 0.1;
constexpr double kAdvanceKd = 0.5;

hiwonder::Brick &brick() {
    if (!g_attached || g_brick == nullptr) {
        throw std::logic_error(
            "spike_compat: llamar a Spike_Attach() antes de usar la API");
    }
    return *g_brick;
}

void require_steering_calibration() {
    if (g_config.pd_output_to_steering == 0.0) {
        throw std::logic_error(
            "spike_compat: pd_output_to_steering sin calibrar; el factor 6.4 "
            "del LEGO no es trasladable al varillaje Ackermann del HX-12H");
    }
}

void require_speed_calibration() {
    if (g_config.rps_at_speed_100 <= 0.0) {
        throw std::logic_error("spike_compat: rps_at_speed_100 sin calibrar");
    }
}

double clamp_unit(double v) { return std::max(-1.0, std::min(1.0, v)); }

/* speed SPIKE (0..100 con signo) -> r/s, con tope duro de seguridad. */
float speed_to_rps(int speed) {
    require_speed_calibration();
    const double rps =
        (static_cast<double>(speed) / 100.0) * g_config.rps_at_speed_100;
    const double limit = static_cast<double>(g_config.max_rps_limit);
    return static_cast<float>(std::max(-limit, std::min(limit, rps)));
}

/* El yaw del hub venía en décimas de grado y el PD se afinó en esas unidades.
   Se conserva la escala para que las ganancias originales signifiquen lo
   mismo: el error entra al PD en decigrados. */
double to_decidegrees(double degrees) { return degrees * 10.0; }

/* Un paso del PD original:  error = s1 - s2;  et = kp*error + kd*(error - ea).
   Aplica dirección y, si se pide, velocidad. Devuelve el error, y lo guarda
   como hacía "global error" en el hub. */
double pd_step(double s1, double s2, int speed, double kp, double kd,
               bool drive_motor, double deadband = 0.0) {
    /* Validar toda la calibracion ANTES de tocar hardware: si no, se movia la
       direccion y solo despues fallaba por la velocidad, dejando el robot a
       medio actuar. */
    require_steering_calibration();
    if (drive_motor) require_speed_calibration();
    const double error = spike_compat::apply_deadband(s1 - s2, deadband);
    const double et = (kp * error) + (kd * (error - g_prev_error));
    g_prev_error = error;

    brick().set_steering(clamp_unit(et * g_config.pd_output_to_steering),
                         g_config.steering);
    if (drive_motor) {
        brick().set_motor_speed(brick().position_motor_id(),
                                speed_to_rps(speed));
    }
    return error;
}

/* Siguiente paso del lazo: espera una muestra NUEVA del IMU (20 Hz). Con un
   periodo fijo, el PD a veces veía el mismo yaw dos veces (D = 0) y otras un
   salto doble (D x2). Si el IMU no llega, no bloquea más de 5 periodos. */
void wait_control_step() {
    brick().wait_for_imu(g_config.control_period * 5);
}

/* Banda muerta del rumbo en las unidades del PD (decigrados). */
double heading_deadband() { return g_config.heading_deadband_deg * 10.0; }

}  // namespace detail

double apply_deadband(double error, double band) {
    if (!(band > 0.0)) return error;
    if (std::fabs(error) <= band) return 0.0;
    return error > 0.0 ? error - band : error + band;
}

void Spike_Attach(hiwonder::Brick *brick_ptr, const Config &config) {
    detail::g_brick = brick_ptr;
    detail::g_config = config;
    detail::g_prev_error = 0.0;
    detail::g_attached = (brick_ptr != nullptr);
}

void Spike_Detach() noexcept {
    detail::g_brick = nullptr;
    detail::g_attached = false;
    detail::g_prev_error = 0.0;
}

bool Spike_Is_Attached() noexcept { return detail::g_attached; }

const Config &Spike_Get_Config() { return detail::g_config; }

}  // namespace spike_compat

/* ------------------------------------------------------------------------ */
extern "C" {

namespace sd = spike_compat::detail;

/* --- Transporte del REPL: obsoleto -------------------------------------- */

int Spike_Serial_Init(void) {
    try {
        sd::brick().open();
        return 0;
    } catch (const std::exception &) {
        return -1;
    }
}

void Spike_Close_Serial(void) {
    if (spike_compat::Spike_Is_Attached()) {
        sd::g_brick->close();
    }
}

void Spike_Send_Serial_Data(char[]) {}
char *Spike_Read_Serial_Data(void) { static char empty[1] = ""; return empty; }
void Spike_Interpreter(void) {}
void Spike_Initialize_Libraries(void) {}
void Spike_End_Function(void) {}

void Spike_Concatenate(int list_lenght, const char *argument_1[], char *buffer) {
    int i = 0;
    for (int j = 0; j < list_lenght; ++j) {
        const char *current_string = argument_1[j];
        int k = 0;
        while (true) {
            buffer[i] = current_string[k];
            if (current_string[k] == '\0') break;
            ++k;
            ++i;
        }
    }
}

/* --- Dirección y parada -------------------------------------------------- */

/* El original distinguía LONGEST_PATH y SHORTEST_PATH porque el motor F daba
   vueltas completas. Un servo bus con topes mecánicos no tiene ese problema:
   ambas variantes centran igual. */
void Spike_Center_Vehicle(void) {
    sd::brick().center_steering(sd::g_config.steering);
}

void Spike_Center_Vehicle_Short(void) { Spike_Center_Vehicle(); }

void Spike_Coast_Motors(void) { sd::brick().stop_all(); }

/* Original: motor.stop(port.F, stop=HOLD) y motor.stop(port.B, stop=HOLD).
   Aquí: HOLD de posición del firmware (MOTOR_EXT v3) en el motor de
   propulsión; el servo de dirección ya sostiene su posición por sí mismo. Si
   el firmware no admite HOLD (anterior a pos-v8), parada como antes. */
void Spike_Hold_Motors(void) {
    auto &b = sd::brick();
    const auto &cfg = sd::g_config;
    try {
        const auto status = b.hold_motor_position(
            cfg.hold_max_rps, cfg.hold_tolerance_ticks, cfg.hold_max_pulse);
        if (status.state == hiwonder::MotorPositionState::holding) return;
    } catch (const std::runtime_error &) {
        /* sin respuesta: se cae a la parada */
    }
    b.stop_all();
}

/* --- Giroscopio ---------------------------------------------------------- */

void Spike_Reset_Gyro(float degrees) {
    sd::brick().reset_yaw(static_cast<double>(degrees));
    sd::g_prev_error = 0.0;
}

float Spike_Get_Gyro(void) {
    return static_cast<float>(sd::brick().imu().yaw_deg);
}

/* --- Lazos no bloqueantes: un paso de PD por llamada --------------------- */

void Spike_Forward(int speed, int reference) {
    sd::pd_step(sd::to_decidegrees(sd::brick().imu().yaw_deg),
                sd::to_decidegrees(static_cast<double>(reference)),
                speed, sd::kForwardKp, sd::kForwardKd, true,
                sd::heading_deadband());
}

void Spike_Follow_Reference(int speed, float reference_1, float reference_2) {
    /* Aquí las referencias son magnitudes del usuario (p. ej. distancias del
       lidar), no ángulos: no se convierten a decigrados. */
    sd::pd_step(static_cast<double>(reference_1),
                static_cast<double>(reference_2),
                speed, sd::kFollowKp, sd::kFollowKd, true);
}

/* --- Bloqueantes --------------------------------------------------------- */

/* Original: reseteaba la posición relativa del motor B, lo empujaba a ciclo de
   trabajo constante y vigilaba en bucle hasta alcanzar los grados, corrigiendo
   rumbo con el PD; luego COAST.

   Aquí la distancia la controla el lazo de posición del STM32
   (MOVE_RELATIVE, validado en banco 2026-08-22), y el host sólo corrige rumbo.
   Es más preciso que el original, pero DIFIERE: no se manda velocidad a la
   propulsión (motor de posición, M4) durante el avance, porque
   set_motor_speed cancelaría el movimiento posicional en el firmware.
   "speed" pasa a ser el tope de velocidad. */
void Spike_Advance_For_Degrees(int speed, int degrees, int reference) {
    auto &b = sd::brick();
    sd::require_steering_calibration();
    sd::require_speed_calibration();
    const auto ticks = static_cast<std::int32_t>(std::llround(
        static_cast<double>(degrees) * sd::g_config.spike_degrees_to_ticks));

    b.zero_motor_position();
    b.move_motor_relative(ticks, sd::speed_to_rps(speed),
                          sd::g_config.position_tolerance_ticks,
                          sd::g_config.position_timeout_ms);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(sd::g_config.position_timeout_ms);
    for (;;) {
        sd::pd_step(sd::to_decidegrees(b.imu().yaw_deg),
                    sd::to_decidegrees(static_cast<double>(reference)),
                    speed, sd::kAdvanceKp, sd::kAdvanceKd, false,
                    sd::heading_deadband());
        const auto status = b.motor_position_status();
        if (status.valid && !status.active) break;
        if (std::chrono::steady_clock::now() > deadline) break;
        sd::wait_control_step();
    }
    Spike_Coast_Motors();
}

namespace {

/* Cuerpo común de turn / small_turn. El original repetía el ciclo de trabajo
   dentro del bucle; aquí la velocidad es de lazo cerrado y basta fijarla una
   vez. Se añade una guarda de tiempo que el original NO tenía: un bucle
   infinito con el motor girando es un riesgo real en banco. */
void turn_impl(int direction, int speed, float degrees, int tire_turn,
               bool small) {
    auto &b = sd::brick();
    if (sd::g_config.tire_turn_to_steering == 0.0) {
        throw std::logic_error(
            "spike_compat: tire_turn_to_steering sin calibrar");
    }
    sd::require_speed_calibration();  /* antes de mover la direccion */
    const double steer = sd::clamp_unit(static_cast<double>(tire_turn) *
                                        sd::g_config.tire_turn_to_steering *
                                        static_cast<double>(direction));
    b.set_steering(steer, sd::g_config.steering);
    b.set_motor_speed(b.position_motor_id(), sd::speed_to_rps(speed));

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(sd::g_config.position_timeout_ms);
    for (;;) {
        const double yaw = b.imu().yaw_deg;
        const bool done = small
            ? !(std::fabs(static_cast<double>(degrees)) <
                static_cast<double>(direction) * yaw)
            : !(std::fabs(static_cast<double>(degrees)) > std::fabs(yaw));
        if (done) break;
        if (std::chrono::steady_clock::now() > deadline) break;
        sd::wait_control_step();
    }
    Spike_Coast_Motors();
    Spike_Hold_Motors();
}

}  // namespace

void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn) {
    turn_impl(direction, speed, degrees, tire_turn, false);
}

void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn) {
    turn_impl(direction, speed, degrees, tire_turn, true);
}

}  // extern "C"
