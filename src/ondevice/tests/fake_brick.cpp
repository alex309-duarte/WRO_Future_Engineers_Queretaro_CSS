#include "fake_brick.h"

#include <string>

namespace fake {

namespace {
Estado g_estado;
std::uint64_t g_secuencia = 0;
// Grados de yaw por muestra del IMU con dirección ±1 y 1 r/s.
constexpr double kGradosPorPaso = 2.0;
}  // namespace

Estado &estado() { return g_estado; }
void reiniciar() { g_estado = Estado{}; }

}  // namespace fake

namespace hiwonder {

using fake::estado;

Brick::Brick(Config config) : config_(config) {}
Brick::~Brick() { close(); }

void Brick::open() {
    fd_ = 1;
    estado().eventos.push_back("open");
}
void Brick::close() noexcept { fd_ = -1; }
bool Brick::is_open() const noexcept { return fd_ >= 0; }

void Brick::set_motor_speed(std::uint8_t, float speed_rps) {
    estado().ultima_velocidad_rps = speed_rps;
}
void Brick::stop_all() {
    estado().ultima_velocidad_rps = 0.0F;
    ++estado().paradas;
}

MotorPositionCapabilities Brick::motor_position_capabilities(std::chrono::milliseconds) {
    MotorPositionCapabilities caps;
    caps.valid = true;
    caps.motor_mask = estado().motor_mask;
    caps.ticks_per_revolution = 1320;
    return caps;
}

MotorPositionStatus Brick::move_motor_relative(std::int32_t delta_ticks, float, std::uint16_t,
                                               std::uint16_t, std::chrono::milliseconds) {
    estado().ultimo_avance_ticks = delta_ticks;
    MotorPositionStatus s;
    s.valid = true;
    return s;
}
MotorPositionStatus Brick::motor_position_status(std::chrono::milliseconds) {
    MotorPositionStatus s;  // el avance termina en cuanto se consulta
    s.valid = true;
    s.active = false;
    return s;
}
MotorPositionStatus Brick::zero_motor_position(std::chrono::milliseconds) {
    MotorPositionStatus s;
    s.valid = true;
    return s;
}
MotorPositionStatus Brick::hold_motor_position(float, std::uint16_t, std::uint16_t,
                                               std::chrono::milliseconds) {
    ++estado().holds;
    MotorPositionStatus s;
    s.valid = true;
    s.state = MotorPositionState::holding;
    return s;
}

void Brick::set_bus_servo_position(std::uint8_t servo_id, std::uint16_t position,
                                   std::uint16_t duration_ms) {
    estado().eventos.push_back("servo " + std::to_string(servo_id) + " " +
                               std::to_string(position) + " " + std::to_string(duration_ms));
}
void Brick::set_steering(double normalized, const SteeringCalibration &calibration) {
    estado().ultima_direccion = normalized;
    estado().ultima_calibracion = calibration;
}
void Brick::center_steering(const SteeringCalibration &calibration) {
    set_steering(0.0, calibration);
    estado().eventos.push_back("centrar");
}

ImuSample Brick::imu() const {
    ImuSample s;
    s.yaw_deg = estado().yaw_deg;
    s.sequence = ++fake::g_secuencia;
    s.valid = true;
    return s;
}
bool Brick::wait_for_imu(std::chrono::milliseconds) const {
    // Una muestra nueva: el robot avanza un paso con la dirección y velocidad
    // vigentes. Derecha (+1) hace bajar el yaw.
    estado().yaw_deg -= estado().ultima_direccion * estado().ultima_velocidad_rps *
                        fake::kGradosPorPaso;
    return true;
}
void Brick::calibrate_gyro(std::chrono::milliseconds duration) {
    estado().eventos.push_back("calibrate " + std::to_string(duration.count()));
}
void Brick::reset_yaw(double yaw_deg) {
    estado().yaw_deg = yaw_deg;
    estado().ultimo_reset_yaw = yaw_deg;
}

}  // namespace hiwonder
